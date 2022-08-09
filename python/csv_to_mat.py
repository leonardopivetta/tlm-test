import os
import scipy.io
from zipfile import *
from tqdm import tqdm
from pandas import read_csv
from browseTerminal import terminalBrowser


def findAllFiles(path, extension):
    dirs = os.listdir(path)
    paths = []
    for dir in dirs:
        if (os.path.isdir(path + "/" + dir)):
            paths += findAllFiles(path + "/" + dir, extension)
        else:
            if (extension in dir):
                paths.append(path + "/" + dir)
    return paths



def compressFolders(paths, zipPath):
    print("COMPRESSING")

    zf = ZipFile(os.path.join(zipPath, "MatFiles.zip"), "w", compression=ZIP_STORED)

    for path in tqdm(paths):
        zf.write(path, path.replace(zipPath, ""))

    size = sum([zinfo.file_size for zinfo in zf.filelist])
    return float(size)


if __name__ == "__main__":
    # browser = terminalBrowser(startPath="/home/")
    # base_path = browser.browse()
    base_path = "/media/filippo/dataset_dv/Vadena Logs_da_sborare/logs/28_03_2022"
    files = findAllFiles(base_path, ".csv")

    print("Found {} csv files.".format(len(files)))
    print("Start conversion")

    to_zip = []
    csv_total_file_size = 0
    mat_total_file_size = 0
    for file in tqdm(files):
        try:
            csv = read_csv(file, sep=',', index_col=False)
        except:
            pass

        values = {}
        for i, col in enumerate(csv.columns):
            if("Unnamed" in col):
                continue
            values[col] = csv.iloc[:,i].values.transpose()

        new_fname = file.replace(".csv", ".mat")
        scipy.io.savemat(new_fname, values)

        to_zip.append(new_fname)
        csv_total_file_size += os.path.getsize(file)
        mat_total_file_size += os.path.getsize(new_fname)

    zip_size = compressFolders(to_zip, base_path)
    print("Path parsed: " + base_path)
    print("CSV total files sizes: {} Mb".format(csv_total_file_size/1000000))
    print("MAT total files sizes: {} Mb".format(mat_total_file_size/1000000))
