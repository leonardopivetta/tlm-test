import os
import re
from sqlite3 import Timestamp
from dotenv import load_dotenv
from influxdb_client import InfluxDBClient, Point, WritePrecision
from influxdb_client.client.write_api import SYNCHRONOUS
from numpy import double
import pandas as pd
import sys
from datetime import datetime
import json

class bcolors:
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKCYAN = '\033[96m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'

warningsEnabled = '-w' in sys.argv
stopOnError = '-e' in sys.argv
force = '-f' in sys.argv

LOGS_STATUS_FILE = './logs_status.csv'

def addFileToLogStatus(file):
    if not os.path.isfile(LOGS_STATUS_FILE) or os.stat(LOGS_STATUS_FILE).st_size == 0:
        with open(LOGS_STATUS_FILE, 'w') as f:
            f.write("PATH,PUSHED\n")
    logs_status = pd.read_csv(LOGS_STATUS_FILE)
    if file not in logs_status["PATH"].unique():
        logs_status = pd.concat([logs_status, pd.DataFrame([[file, False]], columns=["PATH","PUSHED"])])
        logs_status.to_csv(LOGS_STATUS_FILE, index=False)

def initStatusFile(LOGS_PATH):
    for (dirpath, dirnames, filenames) in os.walk(LOGS_PATH):
        for file in filenames:
            if file.endswith('.csv') or file == 'Session.json':
                addFileToLogStatus(dirpath+"/"+file)
                
def checkBucketAndCreate(client: InfluxDBClient):
    logs_status = pd.read_csv(LOGS_STATUS_FILE)
    buckets_required = set()
    print("Checking buckets", end='\r')
    for index, row in logs_status["PATH"].iteritems():
        if not row:
            continue
        match = re.search(r"\d{2}_\d{2}_\d{4}", row)
        if not match:
            continue
        buckets_required.add(match.group())
        logs_status.loc[index, "BUCKET"] = match.group()
    logs_status.to_csv(LOGS_STATUS_FILE, index=False)
    print("Creating buckets", end='\r')
    for bucket in buckets_required:
        buck = client.buckets_api().find_bucket_by_name(bucket)
        if buck is None:
            client.buckets_api().create_bucket(None, bucket, description='test')
    print(bcolors.OKGREEN+"Required buckets created"+bcolors.ENDC)

def pushToInflux(client: InfluxDBClient, LOGS_PATH):
    logs_status = pd.read_csv(LOGS_STATUS_FILE)
    size = len(logs_status.index)
    print("Pushing to influx")
    for index, row in logs_status.iterrows():
        if row["PUSHED"]:
            continue
        file = row["PATH"]
        if not os.path.isfile(file) or os.stat(file).st_size == 0:
            if warningsEnabled:
                print(bcolors.WARNING+"WARNING: "+ file + " does not exist"+bcolors.ENDC)
            continue
        if row["PATH"].split('/')[-1] == 'Session.json':
            write_api = client.write_api(write_options=SYNCHRONOUS, precision=WritePrecision.S)
            jsonFile = json.load(open(file))
            dateFormat = '%d_%m_%Y-%H:%M:%S'
            timeString = '{}-{}'.format(jsonFile["Date"], jsonFile["Time"])
            timestamp = datetime.strptime(timeString, dateFormat).timestamp()
            write_api.write(bucket=row["BUCKET"], record=Point('Session').time(int(timestamp)).field("Session.json", str(jsonFile)), write_precision=WritePrecision.S)
            continue
        try:
            data = pd.read_csv(file, skipfooter=1, engine='python')
        except:
            if warningsEnabled:
                print(bcolors.WARNING+"WARNING: file '"+file.replace(LOGS_PATH+"/", "")+"' is not a valid csv file"+bcolors.ENDC)
            continue
        if not data.empty:
            name = file.split('/')[-1].split('.')[0].replace(" ", "_")
            print("\033[K"+format(index *100 / size, '.2f')+"% \tPushing " + name, end='\r')
            ## TO REMOVE
            # if file.split('/')[-1] == "GPS 0.csv":
            #     data.rename(columns={'timestamp': '_timestamp'}, inplace=True)
            #     def convertStoUS(x):
            #         return int(x * 1000000)
            #     data["_timestamp"] = data["_timestamp"].apply(convertStoUS)
            
            write_api = client.write_api(write_options=SYNCHRONOUS, precision=WritePrecision.US)
            data.set_index('_timestamp', inplace=True)
            try:
                write_api.write(bucket=row["BUCKET"], org=INFLUX_ORG, record=data, data_frame_measurement_name=name, write_precision=WritePrecision.US)
                logs_status.loc[index, "PUSHED"] = True
                logs_status.to_csv(LOGS_STATUS_FILE, index=False)
            except Exception as e:
                print(bcolors.FAIL+bcolors.BOLD+"ERROR: "+file.replace(LOGS_PATH+"/", "")+" could not be pushed to influx"+bcolors.ENDC)
                print(bcolors.OKBLUE, end='')
                print(e, end=bcolors.ENDC+'\n')
                if stopOnError:
                    sys.exit(1)
    print(bcolors.OKGREEN+"\033[KPushed data to influx"+bcolors.ENDC)
    


if __name__ == "__main__":

    for arg in sys.argv:
        if arg not in [sys.argv[0],'-w', '-e', '-f']:
            print("""Usage {} <options>
            -w  Enable warnings
            -e  Stop on error
            -f  Force push of all the csv
            -h  Show this help
            
        ENV VARIABLES REQUIRED (.env file):
            INFLUX_ORG: The organization name in influxdb
            INFLUX_TOKEN: The token with all access privileges in influxdb
            INFLUX_HOST: The host of the influxdb (eg: http://localhost:8086)
            LOGS_PATH: The path to the logs folder (eg: /home/user/logs)
            """.format(sys.argv[0]))
            sys.exit(1)

    # LOADING ENV VARIABLES
    load_dotenv()
    LOGS_PATH = os.getenv('LOGS_PATH')
    INFLUX_TOKEN = os.getenv('INFLUX_TOKEN')
    INFLUX_ORG = os.getenv('INFLUX_ORG')
    INFLUX_SERVER = os.getenv('INFLUX_SERVER')

    client = InfluxDBClient(INFLUX_SERVER, token=INFLUX_TOKEN, org=INFLUX_ORG, timeout=20*60000, retries=3)
    if not client.ping():
        print(bcolors.FAIL+"ERROR: Could not connect to influx server ({})".format(INFLUX_SERVER)+bcolors.ENDC)
        sys.exit(1)
    if force:
        os.remove(LOGS_STATUS_FILE)
    initStatusFile(LOGS_PATH)
    checkBucketAndCreate(client)
    pushToInflux(client, LOGS_PATH)

