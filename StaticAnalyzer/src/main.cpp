#include <iostream>
#include <boost/program_options.hpp>
#include "Analyzer/NFAs_ActivityAnalyzer.hpp"
#include "Analyzer/WPOTrPDS_ActivityAnalyzer.hpp"
#include "Analyzer/NuXmv_ActivityAnalyzer.hpp"
#include "Parser/ASMParser.hpp"
//#include "Analyzer/FragmentAnalyzer.hpp"
using namespace TaskDroid;
using std::cout, std::endl;
namespace po = boost::program_options;
int main (int argc, char* argv[]) {
    po::options_description opts("Allowed options");
    po::variables_map vm;
    int k = 0, h = 0;
    opts.add_options()
    ("help,h", "produce help message")
    ("verify,v", po::value<string>(), "")
    ("act", po::value<string>(), "")
    ("engine,e", po::value<string>()->default_value("nuxmv"), "")
    ("stack-num,k", po::value<int>(&k)->default_value(1), "k")
    ("stack-heigh,b", po::value<int>(&h)->default_value(6), "h")
    ("output-file,o", po::value<string>(), "")
    ("input-files,i", po::value<string>(), "input files")
    ("manifest-input-file,m", po::value<string>(), "manifest file")
    ("fragment-input-file,f", po::value<string>(), "fragment file")
    ("aftm-input-file,a", po::value<string>(), "activity fragment transition model file")
    ("main-activity", po::value<string>(), "activity fragment transition model file")
    ("nuxmv", po::value<string>(), "nuxmv command");
    string manifestFileName = "", aftmFileName = "", fragmentFileName = "", engine = "nuxmv", outputFileName = "out.txt", nuxmvCmd = "nuXmv";
    try {
        po::store(po::parse_command_line(argc, argv, opts), vm);
        po::notify(vm);
        if (vm.count("input-files")) {
            string dir = vm["input-files"].as<std::string>();
            manifestFileName = dir + "/AndroidManifest.txt";
            aftmFileName = dir + "/ictgOpt.xml";
            fragmentFileName = dir + "/SingleObject_entry.xml";
        } else {
            if (vm.count("manifest-input-file")) {
                manifestFileName = vm["manifest-input-file"].as<std::string>();
            } else {
                cout << "no manifest file!" << endl;;
                return 0;
            }
            if (vm.count("aftm-input-file")) {
                aftmFileName = vm["aftm-input-file"].as<std::string>();
            } else {
                cout << "no aftm file!" << endl;;
                return 0;
            }
            if (vm.count("fragment-input-file")) {
                aftmFileName = vm["fragment-input-file"].as<std::string>();
            } else {
                cout << "no fragment file!" << endl;;
                return 0;
            }
        }
        if (vm.count("engine")) {
            engine = vm["engine"].as<std::string>();
        }
        AndroidStackMachine a;
        ASMParser::parseManifest(manifestFileName.c_str(), &a);
        ASMParser::parseATG(aftmFileName.c_str(), &a);
        ASMParser::parseFragment(fragmentFileName.c_str(), &a);
        if (vm.count("main-activity")) {
            string mainActivityName = vm["main-activity"].as<std::string>();
            a.setMainActivity(a.getActivity(mainActivityName));
        }
        a.formalize();
        if (vm.count("output-file")) {
            outputFileName = vm["output-file"].as<std::string>();
        }
        std::ofstream out(outputFileName);
        if (a.getAffinityMap().size() == 0 || !a.getMainActivity()) {
            out << "no graph" << endl;
            return 0;
        }
        a.print(out);
        out << "$Detail: " << endl;
        if (vm.count("verify")) {
             string verify = vm["verify"].as<string>();
             if (verify == "task-bounded") {
                 if (engine == "nfas") {
                     out << "ActNum: " << a.getActivityMap().size() << endl;
                     ID transNum = 0;
                     for (auto& [a, actions] : a.getActionMap())
                         for (auto& action : actions) transNum++;
                     out << "TransNum: " << transNum << endl;
                     transNum = 0;
                     NFAs_ActivityAnalyzer analyzer(&a);
                     out << "NewActNum: " << analyzer.getAlphabet().size() << endl;
                     for (auto& [a, acts] : analyzer.getDelta())
                         for (auto a_ : acts) transNum++;
                     for (auto& [a, acts] : analyzer.getDelta())
                         for (auto a_ : acts) cout << a -> getName() << "->" << a_ -> getName() << endl;
                     out << "NewTransNum: " << transNum << endl;
                     analyzer.analyzeBoundedness(1, out);
                 } else if (engine == "wpotrpds") {
                     out << "ActNum: " << a.getActivityMap().size() << endl;
                     ID transNum = 0;
                     for (auto& [a, actions] : a.getActionMap())
                         for (auto& action : actions) transNum++;
                     out << "TransNum: " << transNum << endl;
                     transNum = 0;
                     WPOTrPDS_ActivityAnalyzer analyzer(&a);
                     out << "NewActNum: " << analyzer.getAlphabet().size() << endl;
                     for (auto& [a, acts] : analyzer.getDelta())
                         for (auto a_ : acts) transNum++;
                     out << "NewTransNum: " << transNum << endl;
                     analyzer.analyzeBoundedness(1, out);
                 } else if (engine == "nuxmv") {
                     out << "ActNum: " << a.getActivityMap().size() << endl;
                     ID transNum = 0;
                     for (auto& [a, actions] : a.getActionMap())
                         for (auto& action : actions) transNum++;
                     out << "TransNum: " << transNum << endl;
                     transNum = 0;
                     NuXmv_ActivityAnalyzer analyzer(&a, h);
                     out << "NewActNum: " << analyzer.getAlphabet().size() << endl;
                     for (auto& [a, acts] : analyzer.getDelta())
                         for (auto a_ : acts) transNum++;
                     out << "NewTransNum: " << transNum << endl;
                     analyzer.analyzeBoundedness(1, out);
                 }
             } else if (verify == "reach") {
                 if (engine == "nfas") {
                     out << "ActNum: " << a.getActivityMap().size() << endl;
                     ID transNum = 0;
                     for (auto& [a, actions] : a.getActionMap())
                         for (auto& action : actions) transNum++;
                     out << "TransNum: " << transNum << endl;
                     transNum = 0;
                     NFAs_ActivityAnalyzer analyzer(&a);
                     out << "NewActNum: " << analyzer.getAlphabet().size() << endl;
                     for (auto& [a, acts] : analyzer.getDelta())
                         for (auto a_ : acts) transNum++;
                     for (auto& [a, acts] : analyzer.getDelta())
                         for (auto a_ : acts) cout << a -> getName() << "->" << a_ -> getName() << endl;
                     out << "NewTransNum: " << transNum << endl;
                     Configuration config(a, "config.td");
                     cout << analyzer.analyzeReachability(config, out) << endl;
                 } else if (engine == "nuxmv") {
                     out << "ActNum: " << a.getActivityMap().size() << endl;
                     ID transNum = 0;
                     for (auto& [a, actions] : a.getActionMap())
                         for (auto& action : actions) transNum++;
                     out << "TransNum: " << transNum << endl;
                     transNum = 0;
                     NuXmv_ActivityAnalyzer analyzer(&a, h);
                     out << "NewActNum: " << analyzer.getAlphabet().size() << endl;
                     // for (auto& [a, acts] : analyzer.getDelta())
                         // for (auto a_ : acts) transNum++;
                     // for (auto& [a, acts] : analyzer.getDelta())
                         // for (auto a_ : acts) cout << a -> getName() << "->" << a_ -> getName() << endl;
                     out << "NewTransNum: " << transNum << endl;
                     Configuration config(a, "config.td");
                     cout << analyzer.analyzeReachability(config, out) << endl;

                 }
             }
        } 
        out.close();
    } catch(string str) {
    }
    return 0;
}
