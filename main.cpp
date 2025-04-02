/*
Copyright (c) 2023 Grid-based Path Planning Competition and Contributors <https://gppc.search-conference.org/>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <cstdio>
#include <ios>
#include <numeric>
#include <algorithm>
#include <string>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <iomanip>
#include "ScenarioLoader.h"
#include "Timer.h"
#include "Entry.h"
#include "validator/ValidatePath.hpp"

#if __linux__
#define GPPC_MEMORY_RECORD
#endif

#ifdef GPPC_MEMORY_RECORD
// track memory to output, only in linux
#include <unistd.h>
#endif

std::string datafile, mapfile, scenfile, flag;
const std::string index_dir = "index_data";
constexpr double PATH_FIRST_STEP_LENGTH = 20.0;
std::vector<bool> mapData;
int width, height;
bool pre   = false;
bool run   = false;
bool check = false;

void LoadMap(const char *fname, std::vector<bool> &map, int &width, int &height)
{
  FILE *f;
  f = std::fopen(fname, "r");
  if (f)
  {
    std::fscanf(f, "type octile\nheight %d\nwidth %d\nmap\n", &height, &width);
    map.resize(height*width);
    for (int y = 0; y < height; y++)
    {
      for (int x = 0; x < width; x++)
      {
        char c;
        do {
          std::fscanf(f, "%c", &c);
        } while (std::isspace(c));
        map[y*width+x] = (c == '.' || c == 'G' || c == 'S');
      }
    }
    std::fclose(f);
  }
}

double euclidean_dist(const xyLoc& a, const xyLoc& b) {
  int dx = std::abs(b.x - a.x);
  int dy = std::abs(b.y - a.y);
  double res = std::sqrt(dx * dx + dy * dy);
  return res;
}

double GetPathLength(const std::vector<xyLoc>& path)
{
  double len = 0;
  for (int x = 0; x < (int)path.size()-1; x++)
    len += euclidean_dist(path[x], path[x+1]);
  return len;
}

// returns -1 if valid path, otherwise id of segment where invalidness was detetcted
int ValidatePath(const std::vector<xyLoc>& thePath)
{
  return inx::ValidatePath(mapData, width, height, thePath);
}

void RunExperiment(void* data) {
  Timer t;
  ScenarioLoader scen(scenfile.c_str());
  std::vector<xyLoc> thePath;

  std::string resultfile = "result.csv";
  std::ofstream fout(resultfile);
  const std::string header = "map,scen,experiment_id,path_size,path_length,ref_length,time_cost,20steps_cost,max_step_time";

  fout << header << std::endl;
  for (int x = 0; x < scen.GetNumExperiments(); x++)
  {
    xyLoc s, g;
    s.x = scen.GetNthExperiment(x).GetStartX();
    s.y = scen.GetNthExperiment(x).GetStartY();
    g.x = scen.GetNthExperiment(x).GetGoalX();
    g.y = scen.GetNthExperiment(x).GetGoalY();

    thePath.clear();
    typedef Timer::duration dur;
    dur max_step = dur::zero(), tcost = dur::zero(), tcost_first = dur::zero();
    bool done = false, done_first = false;
    do {
      t.StartTimer();
      done = GetPath(data, s, g, thePath);
      t.EndTimer();
      max_step = std::max(max_step, t.GetElapsedTime());
      tcost += t.GetElapsedTime();
      if (!done_first) {
        tcost_first += t.GetElapsedTime();
        done_first = GetPathLength(thePath) >= PATH_FIRST_STEP_LENGTH - 1e-6;
      }
    } while (!done);
    double plen = done?GetPathLength(thePath): 0;
    double ref_len = scen.GetNthExperiment(x).GetDistance();


    fout << std::setprecision(9) << std::fixed;
    fout << mapfile  << "," << scenfile       << ","
         << x        << "," << thePath.size() << ","
         << plen     << "," << ref_len        << ","
         << tcost.count() << "," << tcost_first.count() << ","
         << max_step.count() << std::endl;

    if (check) {
      std::printf("%d %d %d %d", s.x, s.y, g.x, g.y);
      int validness = ValidatePath(thePath);
      if (validness < 0) {
        std::printf(" valid");
      } else {
        std::printf(" invalid-%d", validness);
      }
      std::printf(" %d", static_cast<int>(thePath.size()));
      for (const auto& it: thePath) {
        std::printf(" %d %d", it.x, it.y);
      }
      std::printf(" %.5f\n", plen);
    }
  }
}

void print_help(char **argv) {
  std::printf("Invalid Arguments\nUsage %s <flag> <map> <scenario>\n", argv[0]);
  std::printf("Flags:\n");
  std::printf("\t-full : Preprocess map and run scenario\n");
  std::printf("\t-pre : Preprocess map\n");
  std::printf("\t-run : Run scenario without preprocessing\n");
  std::printf("\t-check: Run for validation\n");
}

bool parse_argv(int argc, char **argv) {
  if (argc < 2) return false;
  flag = std::string(argv[1]);
  if (flag == "-full") pre = run = true;
  else if (flag == "-pre") pre = true;
  else if (flag == "-run") run = true;
  else if (flag == "-check") run = check = true;
  else return false;

  if (argc < 3) return false;
  mapfile = std::string(argv[2]);

  if (run) {
    if (argc < 4) return false;
    scenfile = std::string(argv[3]);
  }
  return true;
}

std::string basename(const std::string& path) {
  std::size_t l = path.find_last_of('/');
  if (l == std::string::npos) l = 0;
  else l += 1;
  std::size_t r = path.find_last_of('.');
  if (r == std::string::npos) r = path.size()-1;
  return path.substr(l, r-l);
}

int main(int argc, char **argv)
{

  if (!parse_argv(argc, argv)) {
    print_help(argv);
    return 1;
  }

  bool redirect_output = std::getenv("GPPC_REDIRECT_OUTPUT") != nullptr;
  if (redirect_output) {
    // redirect stdout to file
    std::freopen("run.stdout", "w", stdout);
    std::freopen("run.stderr", "w", stderr);
  }

  // in mapData, 1: traversable, 0: obstacle
  LoadMap(mapfile.c_str(), mapData, width, height);
  datafile = index_dir + "/" + GetName() + "-" + basename(mapfile);

  if (pre)
    PreprocessMap(mapData, width, height, datafile);
  
  if (!run)
    return 0;

  void *reference = PrepareForSearch(mapData, width, height, datafile);

#ifdef GPPC_MEMORY_RECORD
  bool memory_track = std::getenv("GPPC_MEMORY_TRACK") != nullptr;
  char argument[256];
  if (memory_track) {
    std::sprintf(argument, "pmap -x %d | tail -n 1 > run.info", getpid());
    std::system(argument);
  }
#endif
  RunExperiment(reference);
#ifdef GPPC_MEMORY_RECORD
  if (memory_track) {
    std::sprintf(argument, "pmap -x %d | tail -n 1 >> run.info", getpid());
    std::system(argument);
  }
#endif
  return 0;
}
