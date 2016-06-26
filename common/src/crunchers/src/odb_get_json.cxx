#include <cstring>
#include <iostream>
#include "midas.h"  // Data part

int main(int argc, char *argv[])
{
  // Allocations
  HNDLE hDB, hkey;
  char *json_buffer = new char[0x10000];
  int size = 0;
  int bytes_written = 0;
  int rc;

  INT  status, i;
  char host_name[256],exp_name[32], odb_path[256];

  // get default values from environment
  cm_get_environment(host_name, 256, exp_name, 32);

  // parse command line parameters
  for (i=1 ; i<argc ; i++)
    {
    if (argv[i][0] == '-')
      {
      if (i+1 >= argc || argv[i+1][0] == '-')
        goto usage;
      if (argv[i][1] == 'e')
        strcpy(exp_name, argv[++i]);
      else if (argv[i][1] == 'h')
        strcpy(host_name, argv[++i]);
      else if (argv[i][1] == 'q')
        strcpy(odb_path, argv[++i]);
      else
        {
usage:
        printf("usage: test [-h Hostname] [-e Experiment]\n\n");
        return 1;
        }
      }
    }

  // Get the experiment database handle.
  rc = cm_connect_experiment(host_name, exp_name, "json-client", NULL);

  rc = cm_get_experiment_database(&hDB, NULL);

  if (hDB == NULL);

  db_find_key(hDB, 0, odb_path, &hkey);

  if (hkey) {
    db_copy_json_values(hDB, hkey, &json_buffer, &size, &bytes_written, 1, 1, 1);

  } else {

    // Print an empty json tree.
    std::cout << "{}" << std::endl;

    rc = CM_DB_ERROR;
  }

  std::cout << json_buffer << std::endl;

  rc = cm_disconnect_experiment();

  return 1;
}
