#include "Analyzer/ActivityAnalyzer.hpp"
#include "Analyzer/LoopAnalyzer.hpp"
#include "Configuration.hpp"
#include "FragmentTransaction.hpp"
#include "util/util.hpp"
#include <vector>
namespace TaskDroid {

    void ActivityAnalyzer::init() {
        a -> formalize();
        getRealActivities();
        taskNum = realActivities.size();
        mainActivity = a -> getMainActivity();
        mainTaskID = getTaskID(mainActivity);
    }

    ID ActivityAnalyzer::getTaskID(Activity* activity) const {
        return a -> getTaskID(activity -> getAffinity());
    }

    void ActivityAnalyzer::getRealActivities(Activity* realActivity) {
        auto taskID = getTaskID(realActivity);
        Activities work({realActivity}), newWork, newRealActs, visited({realActivity});
        while (work.size()) {
            for (auto& activity : work) {
                if (a -> getActionMap().count(activity) == 0) continue;
                for (auto& [intent, finish] : a -> getActionMap().at(activity)) {
                    auto newActivity = intent -> getActivity();
                    auto newTaskID = getTaskID(newActivity);
                    auto mode = AndroidStackMachine::getMode(activity, intent);
                    if (AndroidStackMachine::isNewMode(activity, intent) && taskID != newTaskID) {
                        if (realActivities[newTaskID].insert(newActivity).second) {
                            newRealActs.insert(newActivity);
                        }
                        outPorts[realActivity].insert(activity);
                    } else {
                        if (visited.insert(newActivity).second) {
                            newWork.insert(newActivity);
                        }
                        if ((mode == PUSH_N || mode == STOP_N) && 
                            realActivity == newActivity && realActivity != mainActivity) continue;
                        reachGraph[realActivity][activity].insert(pair(intent, finish));
                    }
                }
            }
            work.clear();
            if (newWork.size()) {
                work = newWork;
                newWork.clear();
            }
        }
        for (auto activity : newRealActs)
            getRealActivities(activity);
    }

    void ActivityAnalyzer::getRealActivities() {
        realActivities[getTaskID(a -> getMainActivity())].insert(a -> getMainActivity());
        getRealActivities(a -> getMainActivity());
    }

    void ActivityAnalyzer::addReach(Activity* realActivity, Activity* source, Intent* intent, bool finish) {
        if (!reachGraph[realActivity][source].insert(pair(intent, finish)).second) return;
        isReachGraphChanged = true;
        Activity* init = intent -> getActivity();
        auto taskID = getTaskID(realActivity);
        Activities work({init}), newWork, visited({init});
        while (work.size()) {
            for (auto& activity : work) {
                if (a -> getActionMap().count(activity) == 0) continue;
                for (auto& [intent, finish] : a -> getActionMap().at(activity)) {
                    auto newActivity = intent -> getActivity();
                    auto newTaskID = getTaskID(newActivity);
                    if (AndroidStackMachine::isNewMode(activity, intent) && taskID != newTaskID) {
                        outPorts[realActivity].insert(activity);
                    } else {
                        if (visited.insert(newActivity).second) {
                            newWork.insert(newActivity);
                            reachGraph[realActivity][activity].insert(pair(intent, finish));
                        }
                    }
                }
            }
            work.clear();
            if (newWork.size()) {
                work = newWork;
                newWork.clear();
            }
        }
    }

    void ActivityAnalyzer::getReach(const unordered_set<Activity*>& interplays, Activity* cur, 
                                    const unordered_map<Activity*, Activities>& outMap) {
        if (outPorts.count(cur) == 0) return;
        unordered_set<ID> taskIDs;
        for (auto activity : interplays) taskIDs.insert(getTaskID(activity));
        for (auto thisOut : outPorts.at(cur)) {
            for (auto& [intent, finish] : a -> getActionMap().at(thisOut)) {
                auto mode = AndroidStackMachine::getMode(thisOut, intent);
                auto newActivity = intent -> getActivity();
                auto taskID = getTaskID(newActivity);
                for (auto& [realActivity, outs] : outMap) {
                    if ((mode == PUSH_N || mode == STOP_N) && 
                        realActivity == newActivity && realActivity != mainActivity) continue;
                    if (taskID == getTaskID(realActivity) && taskID != getTaskID(cur))
                        for (auto out : outs) {
                            if (mode == STOP_N && newActivity == out) continue;
                            addReach(realActivity, out, intent, finish);
                        }
                }
                if (taskIDs.count(taskID) && taskID != getTaskID(cur)) {
                    unordered_map<Activity*, Activities> newOutMap = outMap;
                    if (newOutMap[cur].insert(thisOut).second)
                        getReach(interplays, newActivity, newOutMap);
                }
            }
        }
    }

    void ActivityAnalyzer::getReach(const unordered_set<Activity*>& interplays,
                                    Activity* main) {
        if (outPorts.count(main) == 0) return;
        for (auto out : outPorts.at(main)) {
            for (auto& [intent, finish] : a -> getActionMap().at(out)) {
                auto newActivity = intent -> getActivity();
                if (interplays.count(newActivity) && newActivity != main) {
                    unordered_map<Activity*, Activities> outMap;
                    outMap[main].insert(out);
                    getReach(interplays, newActivity, outMap);
                }
            }
        }
    }

    void ActivityAnalyzer::getReach(ID k) {
        vector<ID> taskIDs;
        unordered_set<std::unordered_set<ID> > coms;
        for (auto& [id, acts] : realActivities) taskIDs.emplace_back(id);
        if (k + 1 > taskIDs.size())
            k = taskIDs.size() - 1;
        util::combine(taskIDs, k + 1, coms);
        vector<vector<Activity*> > tasks, interplaysVec;
        vector<vector<vector<Activity*> > > interplaysVecs;
        for (auto& taskIDs : coms) {
            tasks.clear();
            interplaysVec.clear();
            for (auto& id : taskIDs) {
                tasks.emplace_back(vector<Activity*>(realActivities.at(id).begin(),
                                                     realActivities.at(id).end()));
            }
            util::product(tasks, interplaysVec);
            interplaysVecs.emplace_back(interplaysVec);
        }
        while(isReachGraphChanged) {
            isReachGraphChanged = false;
            for (auto& interplaysVec : interplaysVecs) {
                for (auto acts : interplaysVec) {
                    unordered_set<Activity*> interplays({acts.begin(), acts.end()});
                    for (auto main : interplays) {
                        getReach(interplays, main);
                    }
                }
            }
        }
    }

    void ActivityAnalyzer::getWeightedGraph(const ActionMap& actionMap,
                                            unordered_map<Activity*, unordered_map<Activity*, Weight> >& graph) {
        for (auto& [activity, actions] : actionMap) {
            for (auto& [intent, finish] : actions) {
                auto mode = AndroidStackMachine::getMode(activity, intent);
                auto newActivity = intent -> getActivity();
                auto& map = graph[activity];
                if (mode == PUSH || mode == PUSH_N ||
                   (activity != newActivity) && 
                   (mode == STOP || mode == STOP_N)) {
                    if (finish) {
                        if (map.count(newActivity) == 0) {
                            map[newActivity] = Weight(0, PUSH, 1);
                        } else {
                            auto weight = map.at(newActivity);
                            map[newActivity] = int(weight) > 0 ? weight : Weight(0, PUSH, 1);
                        }
                    } else {
                        map[newActivity] = Weight(1, PUSH, 0);
                    }
                } else if (activity == newActivity &&
                           (mode == STOP || mode == STOP_N)) {
                    if (finish) {
                        if (map.count(newActivity) == 0) {
                            map[newActivity] = Weight(-1, STOP, 1);
                        } else {
                            auto weight = map.at(newActivity);
                            map[newActivity] = int(weight) > -1 ? weight : Weight(-1, STOP, 1);
                        }
                    } else {
                        if (map.count(newActivity) == 0) {
                            map[newActivity] = Weight(0, STOP, 0);
                        } else {
                            auto weight = map.at(newActivity);
                            map[newActivity] = int(weight) > 0 ? weight : Weight(0, STOP, 0);
                        }
                    }
                } else if (activity != newActivity &&
                        (mode == RTOF || mode == RTOF_N)) {
                    if (finish) {
                        if (map.count(newActivity) == 0) {
                            map[newActivity] = Weight(-1, RTOF, 1);
                        } else {
                            auto weight = map.at(newActivity);
                            map[newActivity] = int(weight) > -1 ? weight : Weight(-1, RTOF, 1);
                        }
                    } else {
                        if (map.count(newActivity) == 0) {
                            map[newActivity] = Weight(0, RTOF, 0);
                        } else {
                            auto weight = map.at(newActivity);
                            map[newActivity] = int(weight) > 0 ? weight : Weight(0, RTOF, 0);
                        }
                    }
                }
            }
        }
    }

    bool ActivityAnalyzer::analyzeBoundedness(ID k, std::ostream& os) {
        if (a -> getActionMap().size() == 0) return false;
        getReach(k);
        unordered_map<Activity*, unordered_map<Activity*, Weight> > graph;
        vector<vector<pair<Activity*, Weight> > > loops;
        LoopsMap loopsMap;
        bool flag = false;
        ID loopNum = 0, traceNum = 0;
        for (auto& [realActivity, actionMap] : reachGraph) {
            graph.clear();
            loops.clear();
            getWeightedGraph(actionMap, graph);
            LoopAnalyzer<Activity*, Weight>::getPositiveLoop(graph, loops);
            if (loops.size()) {
                flag = true;
                for (auto& loop : loops) {
                    loopNum++;
                    // vector<pair<Activity*, Weight> > loop(loop_);
                    // loop.insert(loop.end(), loop_.begin() + 1, loop_.end());
                    vector<ActivityConfiguration> task({ActivityConfiguration(loop[0].first)});
                    for (ID i = 1; i < loop.size(); i++) {
                        auto activity = loop.at(i).first;
                        auto weight = loop.at(i).second;
                        auto w = weight.w;
                        auto mode = weight.mode;
                        auto finish = weight.finish;
                        if (w == 1) {
                            task.insert(task.begin(), ActivityConfiguration(activity));
                        } else if (w == 0) {
                            if (mode == RTOF) {
                                for (auto iter = task.begin(); iter != task.end(); iter++) {
                                    if (iter -> getActivity() == activity) {
                                        task.erase(iter);
                                        break;
                                    }
                                }
                                task.insert(task.begin(), ActivityConfiguration(activity));
                            } else if (mode == PUSH) {
                                task.erase(task.begin());
                                task.insert(task.begin(), ActivityConfiguration(activity));
                            }
                        } else if (w == -1) {
                            if (mode == RTOF) {
                                auto iter = task.begin();
                                for (; iter != task.end(); iter++)
                                    if (iter -> getActivity() == activity) break;
                                if (iter != task.end()) task.erase(iter);
                                task.erase(task.begin());
                                task.insert(task.begin(), ActivityConfiguration(activity));
                            } else if (mode == STOP) {
                                task.erase(task.begin());
                            }
                        }
                    }
                    // for (auto& act : task) cout << act.getActivity() -> getName() << ",";
                    // cout << endl;
                    if (analyzeReachability(Task(realActivity, task), os)) {
                        // cout << "true" << endl;
                        traceNum++;
                    } else {
                        for (auto& [act, w] : loop) cout << act -> getName() << " " << w.w << ",";
                        cout << endl;
                        for (auto ac : task) cout << ac.getActivity() -> getName() << endl;
                        cout << endl;
                        cout << "false" << endl;
                        // return false;
                    }
                }
            }
        }
        // a -> print();
        os << "LoopNum: " << loopNum << endl;
        os << "TraceNum: " << traceNum << endl;
        return flag;
    }

}

