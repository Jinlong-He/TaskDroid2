//
//  ActivityAnalyzer.hpp
//  TaskDroid 
//
//  Licensed under the MIT License <http://opensource.org/licenses/MIT>.
//  SPDX-License-Identifier: MIT
//  Copyright (c) 2021 Jinlong He.
//

#ifndef ActivityAnalyzer_hpp 
#define ActivityAnalyzer_hpp 

#include "../AndroidStackMachine/AndroidStackMachine.hpp"
#include "../AndroidStackMachine/Configuration.hpp"
#include "util/util.hpp"
#include <ostream>
namespace TaskDroid {
    typedef unordered_map<Activity*, vector<vector<Activity*> > > LoopsMap;
    struct Weight {
        Weight()
            : w(0), mode(PUSH), finish(0) {}
        Weight(int w_, ActionMode mode_, bool finish_)
            : w(w_), mode(mode_), finish(finish_) {}
        bool operator==(const Weight& weight) const {
            return w == weight.w && mode == weight.mode && finish == weight.finish;
        }
        bool operator!=(const Weight& weight) const {return !(*this == weight);}
        operator int() const {
            return w;
        }
        friend std::ostream& operator<< (std::ostream& os, const Weight& weight) {
            os << weight.w << " " << weight.mode << " " << weight.finish;
            return os;
        }
        int w;
        ActionMode mode;
        bool finish;
    };
    class ActivityAnalyzer {
    public:
        ActivityAnalyzer()
            : a(nullptr),
              starActivity("*", "", STD),
              dotActivity(".", "", STD),
              concatActivity(",", "", STD),
              taskNum(0),
              isReachGraphChanged(true) {}

        ActivityAnalyzer(AndroidStackMachine* a_)
            : a(a_),
              starActivity("*", "", STD),
              dotActivity(".", "", STD),
              concatActivity(",", "", STD),
              taskNum(0),
              isReachGraphChanged(true) {}

        ID getTaskID(Activity* activity) const;

        virtual bool analyzeReachability(const Configuration& config, std::ostream& os = std::cout) = 0;
        virtual bool analyzeReachability(const Task& task, std::ostream& os = std::cout) = 0;
        bool analyzeBoundedness(ID k, std::ostream& os = std::cout);
    protected:
        AndroidStackMachine* a;
        Activity starActivity;
        Activity dotActivity;
        Activity concatActivity;
        virtual void init();
    private:
        void getReach(ID k);
        void getReach(const unordered_set<Activity*>& interplays, Activity* main);
        void getReach(const unordered_set<Activity*>& interplays, Activity* cur, Activity* main, Activity* out);
        void getReach(const unordered_set<Activity*>& interplays, Activity* cur, 
                      const unordered_map<Activity*, Activities>& outMap);
        void addReach(Activity* realActivity, Activity* source, Intent* intent, bool finish);
        void getWeightedGraph(const ActionMap& actionMap, unordered_map<Activity*, 
                                                          unordered_map<Activity*, Weight> >& graph);

        void getRealActivities();
        void getRealActivities(Activity* realActivity);

    protected:
        unordered_map<ID, Activities> realActivities;
        unordered_map<Activity*, ActionMap> reachGraph;
        unordered_map<Activity*, Activities> outPorts;
        ID taskNum;
        ID mainTaskID;
        Activity* mainActivity;
        bool isReachGraphChanged;
    };
}
#endif /* ActivityAnalyzer_hpp */


