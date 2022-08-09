# Eagle TRT
Log messages received in CAN bus.  
Start and stop logging by sending specific command messages.  

> :warning: **THIS VERSION IS NOT COMPATIBLE WITH LOGS BEFORE 24/05/2021**

## Contents
- [Telemetry](#telemetry)
- [Logger](#logger)
- [Port](#port)
- [CSV](#csv)
- [Checker](#checker)
- [Dashboard](#dashboard)



# Telemetry
## Usage
Run the program.  
To start logging send:  
~~~
0A0#6601
~~~
To stop logging send:
~~~
0A0#6600
~~~

A configuration can me sent when starting logging.  
Can be configured pilot, race and circuit:
~~~
PILOTS = [
  "none",
  "Ivan",
  "Filippo",
  "Mirco",
  "Nicola",
  "Davide"
]
RACES = [
  "none",
  "Autocross",
  "Skidpad",
  "Endurance",
  "Acceleration"
]
CIRCUITS = [
  "none",
  "Vadena",
  "Varano",
  "Povo",
  "Skio"
]
~~~

Send the index of the parameter to be configured.  
Parameter 1: Pilot  
Parameter 2: Race  
Parameter 3: Circuit  
~~~
//       pp rr cc
0A0#6601 01 03 02

// Starts with config Ivan, Endurance, Varano
~~~

pp stands for pilot   vector index expressed in hexadecimal.  
rr stands for race    vector index expressed in hexadecimal.  
cc stands for circuit vector index expressed in hexadecimal.  

So, in the start message, using parameters, the payload contains 4 parameters:
0: start
1: pilot
2: race
3: circuit


## OUTPUT

Logs file can be found in:  
~~~
~/Desktop/logs
~~~
At each start a new sub_directory is created like:  
~~~
20201215_183218_Mirco_Skidpad
Date     Time   Pilot Race
~~~



### CSV
In the sub_folder will be created a file for each sensor/device in the car.  
A file sample:
~~~
timestamp;x;y;z;scale
1630002055.413356;202.030000;-101.210000;-245.540000;8.000000
1630002055.414356;202.030000;-101.210000;-245.540000;8.000000
1630002055.415356;202.030000;-101.210000;-245.540000;8.000000
1630002055.416356;202.030000;-101.210000;-245.540000;8.000000
~~~

### JSON
If the requirement is satisfied will be created **TelemetryInfo.json**.  
~~~
{
  "Date": "Thu Aug 26 20:20:55 2021\n",
  "Pilot": "default",
  "Race": "default",
  "Circuit": "default",
  "Data": {
    "CAN": {
      "Messages": 65174,
      "Average Frequency (Hz)": 14390,
      "Duration (seconds)": 4.529019832611084
    },
    "GPS": {
      "Messages": 1000,
      "Average Frequency (Hz)": 20,
      "Duration (seconds)": 50
    }
  }
}
~~~



# Logger
## Usage
Run the program.  
To start logging send:  
~~~
0A0#6501
~~~
To stop logging send:
~~~
0A0#6500
~~~

A configuration can me sent when starting logging.  
Can be configured pilot, race and circuit:
~~~
PILOTS = [
  "none",
  "Ivan",
  "Filippo",
  "Mirco",
  "Nicola",
  "Davide",
]
RACES = [
  "none",
  "Autocross",
  "Skidpad",
  "Endurance",
  "Acceleration",
]
CIRCUITS = [
  "none",
  "Vadena",
  "Varano",
  "Povo",
  "Skio",
]
~~~

Send the index of the parameter to be configured.  
Parameter 1: Pilot  
Parameter 2: Race  
Parameter 3: Circuit  
~~~
//      pprrcc
0A0#6501010302
~~~

pp stands for pilot   vector index expressed in hexadecimal.  
rr stands for race    vector index expressed in hexadecimal.  
cc stands for circuit vector index expressed in hexadecimal.  

So, in the start message, using parameters, the payload contains 4 parameters:
0: start
1: pilot
2: race
3: circuit


## OUTPUT

Logs file can be found in:  
~~~
~/Desktop/logs
~~~
At each start a new sub_directory is created like:  
~~~
20201215_183218_Mirco_Skidpad
Date     Time   Pilot Race
~~~

### CANDUMP
In the sub_folder will be created a file named **candump.log**.  
A file sample:  
~~~
*** EAGLE-TRT
*** Telemetry Log File
*** Tue May 11 16:15:52 2021

*** Pilot: Filippo
*** Race: Autocross
*** Circuit: Vadena

(1620742552.534499)	vcan0	4C5#B2234225
(1620742552.535561)	vcan0	1DF#2732285B85ADF70B
(1620742552.536678)	vcan0	0B9#F1EF2A213FC5D83A
(1620742552.537935)	vcan0	739#9EC0966545489804
(1620742552.539073)	vcan0	7D1#204E31137A7DAA7B
(1620742552.540215)	vcan0	135#F8F5D007F86B
(1620742552.541311)	vcan0	697#B842E511A8109277
(1620742552.542465)	vcan0	670#DB1B706B36FB9E1A
~~~

### JSON
If the requirement is satisfied will be created **LoggerInfo.json**.  
~~~
{
  "Date": "Thu Aug 26 20:20:55 2021\n",
  "Pilot": "default",
  "Race": "default",
  "Circuit": "default",
  "Data": {
    "CAN": {
      "Messages": 65174,
      "Average Frequency (Hz)": 14390,
      "Duration (seconds)": 4.529019832611084
    },
    "GPS": {
      "Messages": 1000,
      "Average Frequency (Hz)": 20,
      "Duration (seconds)": 50
    }
  }
}
~~~

# Port
Shares data to pipes so other processes can read it.  
It was implemented to share data from a serial-port to multiple processes, like two loggers.
# Usage
Run
~~~
sudo ./bin/share
~~~
It can share the same data to **N** files, the number can be specified in **port/share.cpp** (default 2).  
It will open files in **/home/gps** with the n appended at the and.  
So if **N is 3** the files will be called:
- /home/gps0
- /home/gps1
- /home/gps2  

To test if it is working use:
~~~
./bin/read <endname>
~~~
Where <endname> will be **gps0** or **gps1** or **gps2** (if N is 3).

To open a file from your process use:
~~~
mkfifo(filename, 0666);
fd = open(filename, O_RDONLY);
~~~
And simply read it.  
Example can be found in **port/reader.cpp**



# CSV
Tool made to parse CANDUMP files in csv. It creates a CSV file for each device defined in CAN bus.  
The input files must be formatted like the telemetry output files.  

## Usage
Run:
~~~
./bin/csv
~~~
The program needs you to select only one folder containing some **.log** files.  
The algorithm searches in all sub-directories.  
It will start parsing files using all hardware threads available.  

The log must be formatted like it does [Logger](#logger).

## Output
The generated CSV files are in the same folder as the **.log** file.  
If the **.log** file has an integer name (0.log, 1.log, ...) it will create a folder called as the integer number, otherwise will create a **parsed** folder.
In the created folder can be found all the **.csv** files names like the CAN bus device.

> It skips some lines (20-30) at the beginning of the file


# Checker
Given a folder it will find all CAN logs and parses only few messages to   output two files:
- Device.messages  
- Device.json  

.messsages contains the raw messages.  
.json contains the corresponding raw message parsed, so has all the values that the message contains.

## Usage
Run
~~~
./bin/checker
~~~
Select a folder and wait for it to end.  
For each file it will create a folder called "filename_checker" wich contains two file for each device.



# Dashoard
Parses a CAN log file and replays it.  
Parses each message and at the timeout it sends a JSON to a server, from there it will be ready to be shown as real-time graph.

## Usage
Run
~~~
./bin/dashboard
~~~
The two timeouts are specified in the dashboard.h file.
