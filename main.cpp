#include <cstdio>
#include <ios>
#include <numeric>
#include <algorithm>
#include <string>
#include <unistd.h>
#include <cmath>
#include <iostream>
#include <iomanip>
#include "ScenarioLoader.h"
#include "Timer.h"
#include "Entry.h"
#include "validator/ValidatePath.hpp"

std::string datafile, mapfile, scenfile, flag;
const std::string index_dir = "index_data";
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
  double dx = b.x - a.x;
  double dy = b.y - a.y;
  double res = std::sqrt(dx*dx + dy*dy);
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
    dur max_step = dur::zero(), tcost = dur::zero(), tcost20 = dur::zero();
    bool done = false;
    int call_num = 0;
    do {
      t.StartTimer();
      done = GetPath(data, s, g, thePath);
      t.EndTimer();
      max_step = max(max_step, t.GetElapsedTime());
      tcost += t.GetElapsedTime();
      if (thePath.size() <= 20 || call_num == 0) tcost20 += t.GetElapsedTime();
      call_num++;
    } while (!done);
    double plen = done?GetPathLength(thePath): 0;
    double ref_len = scen.GetNthExperiment(x).GetDistance();


    fout << std::setprecision(9) << std::fixed;
    fout << mapfile  << "," << scenfile       << ","
         << x        << "," << thePath.size() << ","
         << plen     << "," << ref_len        << ","
         << tcost.count() << "," << tcost20.count() << "," 
         << max_step.count() << std::endl;
    
    // do basic check and print to stderr if problem
    for (int i = 0, ie = static_cast<int>(thePath.size()); i < ie; ++i) {
      xyLoc pos = thePath[i];
      if (pos.x < -1e-4 || pos.x > (static_cast<double>(width) + 1e-4) || pos.y < -1e-4 || pos.y > (static_cast<double>(height) + 1e-4)) {
        std::fprintf(stderr, "Scenario %d point %d out-of-bounds (%f,%f)\n", x, i, pos.x, pos.y);
      }
      if (i+1 != ie) {
        xyLoc pos2 = thePath[i];
        if (euclidean_dist(pos, pos2) < inx::MIN_SEGMENT_LENGTH - 1e-4) {
          std::fprintf(stderr, "Scenario %d segment %d too short (%f,%f)-(%f,%f)\n", x, i, pos.x, pos.y, pos2.x, pos2.y);
        }
      }
    }

    if (check) {
      std::printf("%f %f %f %f", s.x, s.y, g.x, g.y);
      int validness = ValidatePath(thePath);
      if (validness < 0) {
        std::printf(" valid");
      } else {
        std::printf(" invalid-%d", validness);
      }
      std::printf(" %d", static_cast<int>(thePath.size()));
      for (const auto& it: thePath) {
        std::printf(" %f %f", it.x, it.y);
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
  if (flag== "-full") pre = run = true;
  else if (flag == "-pre") pre = true;
  else if (flag == "-run") run = true;
  else if (flag == "-check") run = check = true;

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

  // redirect stdout to file
  std::freopen("run.stdout", "w", stdout);
  std::freopen("run.stderr", "w", stderr);

  if (!parse_argv(argc, argv)) {
    print_help(argv);
    std::exit(1);
  }

  // in mapData, 1: traversable, 0: obstacle
  LoadMap(mapfile.c_str(), mapData, width, height);
  datafile = index_dir + "/" + GetName() + "-" + basename(mapfile);

  if (pre)
    PreprocessMap(mapData, width, height, datafile);
  
  if (!run)
    return 0;

  void *reference = PrepareForSearch(mapData, width, height, datafile);

  char argument[256];
  std::sprintf(argument, "pmap -x %d | tail -n 1 > run.info", getpid());
  std::system(argument);
  RunExperiment(reference);
  std::sprintf(argument, "pmap -x %d | tail -n 1 >> run.info", getpid());
  std::system(argument);
  return 0;
}
