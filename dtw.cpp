#include <iostream>
#include <stdexcept>

#include <dtw_parm.h>
#include <dtw_util.h>
#include <cmdparser.h>

using namespace DtwUtil;

enum DTW_TYPE {
  SEGMENT_BASED,
  SLOPE_CONSTRAINT,
  FREE_FRAME,
  FIX_FRAME,
  SEGEMENTAL
};

DtwRunner* newRunner(DTW_TYPE type, int dist_type);
vector<string> readFeatures(const string &fn);
void read_file_list(vector<string> &fnames, const string &fn);

vector<IPair*> readTimeSpans(const string& fn, size_t N);
void read_time_spans(vector<IPair*> &times, const string &fn);
inline IPair* seperateByHyphen(const string& s);
void freeTimeSpans(vector<IPair*> &times);

void showDetailedResult(
    const string& q_name, const string& d_name,
    const DtwParm& q_parm, const DtwParm& d_parm,
    const vector<float> &hypo_score,
    const vector<pair<int, int> > hypo_bound);

int main(int argc, char* argv[]) {

  CmdParser cmd(argc, argv);

  cmd.add("-f1", "Specify the filename of feature 1. (in *.mfc, *.fbank)\n"
		 ", or the filename of a list of features (in *.lst, *.scp).")
     .add("-f2", "Same as option -f1.\n")
     .add("-t1", "Specify the time boundary (seperated by hyphen) for -f1. For "
		 "example: 0.5-0.8 . Empty string \"\" means full time span. \n"
		 "If -f1 is something like *.lst or *.scp, then specify a file "
		 "containing all the time durations (in *.txt). \nIn this file,"
		 "you can use either a whitespace \" \" or a hyphen - .",
		 "")
     .add("-t2", "Same as options -t1", "");

  cmd.addGroup("General options for dynamic time warping:")
     .add("--type", "Type of dynamic time warping:\n"
		    "0 -- SegDtwRunner\n"
		    "1 -- SlopeConDtwRunner\n"
		    "2 -- FreeFrameDtwRunner\n"
		    "3 -- FixFrameDtwRunner\n"
		    "4 -- SegementalDtwRunner", "1")
     .add("--dist", "Type of local distance measure:\n"
	            "0 -- Euclidean distance\n"
		    "1 -- Log inner product\n", "0")
     .add("--detail", "Show detailed information about all hypothesized regions"
		      ". If false, show best warping score only.", "false");

  cmd.addGroup("Options for SegDtwRunner:")
     .add("--bseg-ratio", "bseg_ratio", "0.5")
     .add("--superseg-ratio", "superseg_ratio", "4.0")
     .add("--gran", "gran", "3")
     .add("--width", "width", "3");

  cmd.addGroup("Other options:")
     .add("--nsnippet", "Number of hypothesized region to find. "
			"(for type 1~4 only)", "5");

  cmd.addGroup("Example usage:")
     .addGroup("  1) ./dtw -f1 data/iv1_1.fbank -f2 data/iv2_1.fbank")
     .addGroup("  2) ./dtw -f1 data/iv1_1.fbank -f2 data/iv2_1.fbank -t1 10-20 -t2 0-80")
     .addGroup("  3) ./dtw -f1 data/f1.scp -f2 data/f2.scp")
     .addGroup("  4) ./dtw -f1 data/f1.scp -f2 data/f2.scp -t1 data/t1.txt -t2 data/t2.txt");

  if (!cmd.isOptionLegal())
    cmd.showUsageAndExit();

  string f1	= cmd["-f1"];
  string f2	= cmd["-f2"];
  string t1	= cmd["-t1"];
  string t2	= cmd["-t2"];
  bool detail	= cmd["--detail"];
  DTW_TYPE type = (DTW_TYPE) ((int) cmd["--type"]);
  int dist_type	= cmd["--dist"];

  float bseg_ratio	= cmd["--bseg-ratio"];
  float superseg_ratio	= cmd["--superseg-ratio"];
  int gran		= cmd["--gran"];
  int width 		= cmd["--width"];

  int nsnippet		= cmd["--nsnippet"];

  FrameDtwRunner::nsnippet_ = nsnippet;

  // q and d stands for query and document respectively.
  auto q_fnames = readFeatures(f1),
       d_fnames = readFeatures(f2);

  auto q_times  = readTimeSpans(t1, q_fnames.size()),
       d_times  = readTimeSpans(t2, d_fnames.size());

  DtwRunner* runner = newRunner(type, dist_type);

  DtwParm q_parm, d_parm;
  vector<float> hypo_score;
  vector<pair<int, int> > hypo_bound;

  cout << endl;
  for (size_t i=0; i<q_fnames.size(); ++i) {
    for (size_t j=0; j<d_fnames.size(); ++j) {

      if (type == SEGMENT_BASED) {
	q_parm.LoadParm(q_fnames[i], bseg_ratio, superseg_ratio, width, gran, "");
	d_parm.LoadParm(d_fnames[j], bseg_ratio, superseg_ratio, width, gran, "");
      }
      else {
	q_parm.LoadParm(q_fnames[i]);
	d_parm.LoadParm(d_fnames[j]);
      }

      runner->InitDtw(
	  &hypo_score,
	  &hypo_bound,	/* (start, end) frame */
	  NULL,		/* do not backtracking */
	  &q_parm,
	  &d_parm,
	  q_times[i],	/* NULL for full time span */
	  d_times[j]	/* NULL for full time span */
	  );

      runner->DTW();

      if (detail)
	showDetailedResult(q_fnames[i], d_fnames[j],
	    q_parm, d_parm, hypo_score, hypo_bound);
      else {
	if (hypo_score.empty())
	  cout << -float_inf << " ";
	else
	  cout << hypo_score[0] << " ";
      }

      hypo_score.clear();
      hypo_bound.clear();
    }

    cout << endl;
  }

  freeTimeSpans(q_times);
  freeTimeSpans(d_times);

  return 0;
}

DtwRunner* newRunner(DTW_TYPE type, int dist_type) {

  DtwUtil::VectorDistFn dist_fn;
  switch (dist_type) {
    case 0:
      cout << "Using Euclidean distance" << endl;
      dist_fn = DtwUtil::euclinorm;
      break;
    case 1:
      cout << "Using Log inner-product" << endl;
      dist_fn = DtwUtil::innernorm;
      break;
    default:
      cerr << "No such local distance type" << endl;
      exit(-1);
  }

  switch (type) {
    case SEGMENT_BASED:
      cout << "Using Segment-based DTW runner" << endl;
      return new SegDtwRunner(dist_fn);
      break;
    case SLOPE_CONSTRAINT:
      cout << "Using Slope-Constrained DTW runner" << endl;
      return new SlopeConDtwRunner(dist_fn);
      break;
    case FREE_FRAME:
      cout << "Using Free-Frame DTW runner" << endl;
      return new FreeFrameDtwRunner(dist_fn);
      break;
    case FIX_FRAME:
      cout << "Using Fix-Frame DTW runner" << endl;
      return new FixFrameDtwRunner(dist_fn);
      break;
    case SEGEMENTAL:
      cout << "Using Segmental DTW runner" << endl;
      return new SegmentalDtwRunner(dist_fn);
      break;
    default:
      cerr << "No such type of dtw runner" << endl;
      exit(-1);
  }
}

void showDetailedResult(
    const string& q_name, const string& d_name,
    const DtwParm& q_parm, const DtwParm& d_parm,
    const vector<float> &hypo_score,
    const vector<pair<int, int> > hypo_bound) {

  printf("\n");
  printf("+-------------------------------+--------+\n");
  printf("|            Filename           | frames |\n");
  printf("+-------------------------------+--------+\n");
  printf("| %29s | %6d |\n", q_name.c_str(), q_parm.Feat().LT());
  printf("| %29s | %6d |\n", d_name.c_str(), d_parm.Feat().LT());
  printf("+-------------------------------+--------+\n");

  for (size_t i = 0; i < hypo_score.size(); ++i) {
    cout << "hypothesized region[" << i << "]: score = " << hypo_score[i]
      << ", time span = (" << hypo_bound[i].first
      << ", " << hypo_bound[i].second << ")\n";
  }
}

vector<string> readFeatures(const string &fn) {
  vector<string> fnames;
  cout << "Reading file " << fn << " ...\t";

  // Check for file extension
  string ext = fn.substr(fn.find_last_of("."));

  if ( ext == ".lst" || ext == ".scp" )
    read_file_list(fnames, fn);
  else if ( ext == ".mfc" || ext == ".fbank" )
    fnames.push_back(fn);
  else {
    cerr << "Unknown file extension: \"" << ext << "\"" << endl;
    exit(-1);
  }

  cout << fnames.size() << " feature found." << endl;

  return fnames;
}

void read_file_list(vector<string> &fnames, const string &fn) {
  std::ifstream fin(fn.c_str());
  
  if (!fin.is_open()) {
    cerr << "File \"" << fn << "\" not found or permission denied." << endl;
    exit(-1);
  }

  string token;
  while (fin >> token)
    fnames.push_back(token);

  fin.close();
}

inline IPair* getTimeSpan(const string& line) {
  if (line.empty())
    return NULL;

  size_t pos = line.find_first_of("- ");
  if (pos == string::npos)
    throw std::runtime_error("\"" + line + "\" wrong format!!");

  return new IPair(stoi(line.substr(0, pos)), stoi(line.substr(pos+1)));
}

vector<IPair*> readTimeSpans(const string& fn, size_t N) {
  vector<IPair*> times(N, NULL);
  
  size_t pos = fn.find_last_of(".");
  string ext = (pos != string::npos) ? fn.substr(pos) : "";

  if ( ext == ".txt" )
    read_time_spans(times, fn);
  else
    times[0] = getTimeSpan(fn);

  /*for (size_t i=0; i<N; ++i) {
    cout << "#" << i << "\t";
    if (times[i])
      cout << times[i]->first << " ~ " << times[i]->second << endl;
    else
      cout << "-inf ~ +inf" << endl;
  }*/

  return times;
}

void read_time_spans(vector<IPair*> &times, const string &fn) {
    ifstream fin(fn.c_str());

    if (!fin.is_open()) {
      cerr << "File \"" << fn << "\" not found or permission denied." << endl;
      exit(-1);
    }

    string line;
    size_t i = 0;

    while ( std::getline(fin, line) ) {
      if ( i >= times.size() ) {
	clog << "\33[33m[Warning]\33[0m # of rows in " << fn
	     << " exceeds # of rows in feature *.lst or *.scp" << endl;
	return;
      }

      try {
	times[i++] = getTimeSpan(line);
      }
      catch (const std::runtime_error& e) {
	cerr << "\33[31m[Error]\33[0m " << fn << ":" << i << ": " << e.what() << endl;
	exit(-1);
      }
    }
}

void freeTimeSpans(vector<IPair*> &times) {
  for (size_t i=0; i<times.size(); ++i)
    delete times[i];
  times.clear();
}
