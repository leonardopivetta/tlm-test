#include <vector>
#include <thread>
#include <stdio.h>
#include <fstream>
#include <stdlib.h>
#include <string.h>
#include <exception>

#include "utils.h"
#include "browse.h"
#include "vehicle.h"

#include "report.h"

#include "json_loader.h"

using namespace std;

csv_parser_config config;
/**
 * Total lines parse
 */
long long int total_lines = 0;
/**
 * Parse the given file
 *
 * @param filename of the file to be parsed
 */
void parse_file(string fname);
/**
 * Parses all the files in the vector
 *
 * @param files vector of filenames
 */
void parse_files(vector<string> files);

// Add here a thread when is started
vector<thread *> active_threads;

int main()
{

  string home = getenv("HOME");
  string config_path = home + "/csv_parser_config.json";
  if (fs::exists(config_path))
  {
    LoadJson(config, config_path);
  }
  else
  {
    config.subfolder_name = "Parsed";
    config.parse_gps = true;
    config.parse_candump = true;
    config.generate_report = true;
    SaveJson(config, config_path);
  }

  // User select a folder
  Browse b;
  b.SetMaxSelections(10);
  b.SetExtension("*");
  b.SetSelectionType(SelectionType::sel_folder);
  auto selected_paths = b.Start();

  if (selected_paths.size() <= 0)
  {
    cout << "No file selected... exiting" << endl;
    return -1;
  }

  cout << "Selected paths: " << endl;
  for (auto sel : selected_paths)
    cout << "\t" << get_colored(sel, 9, 1) << endl;
  cout << endl;

  // Start the timer
  time_point t_start = high_resolution_clock::now();
  for (string path : selected_paths)
  {
    // Get all the files with that extension
    auto files = get_all_files(path, ".log");

    // Select only candump files
    auto candump_files = get_candump_from_files(files);
    if (candump_files.size() <= 0)
    {
      cout << "No candump found... exiting" << endl;
      return -1;
    }

    // Divide all files for number of cores available.
    int chunks = thread::hardware_concurrency();
    // chunks = 1;
    if (candump_files.size() < chunks)
      chunks = candump_files.size();
    int increment = candump_files.size() / chunks;
    int i0 = 0;
    int i1 = increment;
    for (int i = 0; i < chunks; i++)
    {
      if (i0 > i1 || i1 > candump_files.size())
        continue;

      // Divide in chunks
      vector<string>::const_iterator first = candump_files.begin() + i0;
      vector<string>::const_iterator last = candump_files.begin() + i1;
      vector<string> chunk(first, last);
      if (chunk.size() <= 0)
        continue;

      // Assign to every thread the chunk
      thread *new_thread = new thread(parse_files, chunk);
      active_threads.push_back(new_thread);
      cout << get_colored("Starting thread: " + to_string(i) + " of: " + to_string(chunks), 3) << endl;

      i0 += increment;
      i1 += increment;
    }
    cout << endl;
  }
  // Wait all the threads
  for (auto t : active_threads)
    t->join();

  // Debug
  double dt = duration<double, milli>(high_resolution_clock::now() - t_start).count() / 1000;
  cout << endl;
  cout << get_colored("Total Execution Time: " + to_string(dt) + " seconds", 2) << endl;
  cout << get_colored("Parsed " + to_string(total_lines) + " lines!!", 2) << endl;
  cout << get_colored("Average " + to_string(int(total_lines / dt)) + " lines/sec!!", 2) << endl;

  return 0;
}

void parse_files(vector<string> files)
{
  for (auto file : files)
    parse_file(file);
}

// parse a candump file and a related gps file (if exists)
void parse_file(string fname)
{
  int can_lines = 0;
  int gps_lines = 0;
  int lines_count = 0;
  Report report;

  string out_folder;
  string base_folder = get_parent_dir(fname);

  auto files = get_all_files(base_folder, ".log");
  auto gps_files = get_gps_from_files(files);

  session_config session;
  auto json_files = get_all_files(base_folder, ".json");
  auto session_files = get_files_with_word(json_files, "Session");

  if (session_files.size() != 0)
  {
    try
    {
      LoadJson(session, session_files[0]);
    }
    catch (exception e)
    {
      cout << "Exception loading json " << e.what() << endl;
    }
  }

  // If the filename has an incremental name create a folder with that number
  // Otherwise create parsed folders
  try
  {
    int n = stoi(remove_extension(fname));
    out_folder = base_folder + "/" + to_string(n);
  }
  catch (std::exception &e)
  {
    out_folder = base_folder + "/" + config.subfolder_name;
  }

  create_directory(out_folder);

  Chimera chimera;

  // Add csv files for each device
  // Open them
  // Write CSV header (column name)
  chimera.add_filenames(out_folder, ".csv");
  chimera.open_all_files();
  chimera.write_all_headers(0);

  // Get all lines
  message msg;
  vector<string> lines;
  get_lines(fname, &lines);

  // Contains devices modified from the CAN message
  vector<Device *> modifiedDevices;

  // Start Timer
  double t_start = get_timestamp_u();
  double prev_timestsamp;
  if (config.parse_candump)
  {
    for (uint32_t i = 20; i < lines.size(); i++)
    {
      // Try parsing the line
      try
      {
        if (!parse_message(lines[i], &msg))
          continue;
      }
      catch (exception e)
      {
        continue;
      }
      // Fill the devices
      chimera.parse_message(msg.timestamp, msg.id, msg.data, msg.size, modifiedDevices);

      if (prev_timestsamp > msg.timestamp)
      {
        cout << fname << "\n";
        cout << std::fixed << setprecision(9) << msg.timestamp << "\t";
        cout << lines[i] << "\t" << i << endl;
      }

      // For each device modified write the values in the csv file
      for (auto modified : modifiedDevices)
      {
        *modified->files[0] << modified->get_string(",") + "\n";
        if (config.generate_report)
          report.AddDeviceSample(&chimera, modified);
      }
      prev_timestsamp = msg.timestamp;
    }
  }
  can_lines = lines.size();

  if (config.parse_gps)
  {
    for (auto gps_file : gps_files)
    {
      Gps *current_gps;

      if (fs::path(gps_file).filename().string().find("1") != string::npos)
        current_gps = chimera.gps2;
      else
        current_gps = chimera.gps1;

      get_lines(gps_file, &lines);
      gps_message msg;
      for (size_t i = 20; i < lines.size(); i++)
      {
        try
        {
          if (!parse_gps_line(lines[i], &msg))
            continue;
        }
        catch (exception e)
        {
          cout << "failed parsing gps line: " << lines[i] << endl;
          continue;
        }

        int ret = chimera.parse_gps(current_gps, msg.timestamp, msg.message);
        if (ret == 1)
        {
          *current_gps->files[0] << current_gps->get_string(",") + "\n";
          if (config.generate_report)
            report.AddDeviceSample(&chimera, current_gps);
        }
      }
    }
  }
  gps_lines = lines.size();

  // Debug
  lines_count = can_lines + gps_lines;
  double dt = get_timestamp_u() - t_start;
  cout << "Parsed " << lines_count << " lines in: " << to_string(dt) << " -> " << lines_count / dt << " lines/sec" << endl;
  chimera.close_all_files();

  // Increment total lines
  total_lines += lines_count;

  if (config.generate_report && can_lines > 1000)
  {
    t_start = get_timestamp_u();
    report.Clean(1920 * 2);
    cout << "Generating: " << base_folder << endl;
    try
    {
      report.Generate(base_folder + "/Report.pdf", can_stat);
    }
    catch (exception e)
    {
      cout << "Exception: " << e.what() << " failed generating report: " << base_folder << endl;
    }
    cout << "Generating Report took: " << (get_timestamp_u() - t_start) << " " << base_folder << endl;
  }
}
