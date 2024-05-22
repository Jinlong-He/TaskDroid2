#include "Analyzer/NFAs_ActivityAnalyzer.hpp"
#include "atl/detail/finite_automaton/finite_automaton.hpp"
using std::cout, std::endl, std::to_string;
using namespace atl;
namespace TaskDroid {
    void NFAs_ActivityAnalyzer::init() {
        ActivityAnalyzer::init();
        Activities work({mainActivity}), newWork;
        while (work.size()) {
            for (auto activity : work) {
                if (!alphabet.insert(activity).second) continue;
                if (a -> getActionMap().count(activity) == 0) continue;
                auto& activities = delta[activity];
                for (auto& [intent, finish] : a -> getActionMap().at(activity)) {
                    auto newActivity = intent -> getActivity();
                    auto lmd = newActivity -> getLaunchMode();
                    if (lmd != SIT && !finish) {
                        auto mode = AndroidStackMachine::getMode(activity, intent);
                        if ((mode == CTOP_N && lmd == STK) || (mode == PUSH) || (mode == STOP)) {
                            activities.insert(newActivity);
                            newWork.insert(newActivity);
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
        Activities lambda;
        if (mainActivity -> getLaunchMode() == STK) 
            lambda = Activities({mainActivity});
        NFA_fml* nfa = new NFA_fml();
        auto initState = getNFAState("q0", nullptr, Activities(), nfa);
        auto finalState = getNFAState("qf", mainActivity, lambda, nfa);
        set_initial_state(*nfa, initState);
        set_final_state(*nfa, finalState);
        add_transition(*nfa, initState, finalState, mainActivity);
        //auto transition = add_transition(*nfa, initState, finalState, mainActivity).first;
        //transitionMap[nfa][mainActivity].emplace_back(transition);
        mkLocalNFA(mainTaskID, nfa);
        addNFAs(NFAs({pair(mainTaskID, nfa)}));
        mkNFAs();
        // cout << nfaRepresentations.size() << endl;
    }

    bool NFAs_ActivityAnalyzer::isSITFree() const {
        for (auto& [name, activity] : a -> getActivityMap())
            if (activity -> getLaunchMode() == SIT) return false;
        for (auto& [activity, actions] : a -> getActionMap())
            for (auto& [intent, finish] : actions) {
                if (intent -> getFlags().size() > 0) return false;
                if (finish) return false;
            }
        return true;
    }

    bool NFAs_ActivityAnalyzer::analyzeReachability(const Configuration& config, std::ostream& os) {
        vector<Activity*> word;
        vector<pair<ID, vector<Activity*> > > words;
        for (auto& task : config.getContent()) {
            word.clear();
            for (auto& ac : task.getContent()) {
                auto activity = ac.getActivity();
                if (alphabet.count(activity) == 0) return false;
                word.emplace_back(activity);
            }
            if (alphabet.count(task.getRealActivity()) == 0) return false;
            words.emplace_back(pair(getTaskID(task.getRealActivity()), word));
        }
        for (auto& nfas : nfaRepresentations) {
            if (nfas.size() != words.size()) continue;
            ID i = 0;
            for (i = 0; i < nfas.size(); i++) {
                auto& [id, nfa] = nfas.at(i);
                auto& [id_, word] = words.at(i);
                if (id != id_) break;
                if (!accept(*nfa, word)) break;
            }
            if (i == nfas.size()) return true;
        }
        return false;
    }

    bool NFAs_ActivityAnalyzer::analyzeReachability(const Task& task, std::ostream& os) {
        for (auto& activityConfig : task.getContent())
            if (alphabet.count(activityConfig.getActivity()) == 0) return false;
        NFA_fml nfa;
        mkNFA(task, nfa);
        for (auto& nfas : nfaRepresentations) {
            for (auto& [taskID, nfa_] : nfas) {
                if (taskID == getTaskID(task.getRealActivity())) {
                    if (!is_empty(nfa&*nfa_)) {
                        return true;
                    }
                }
            }
        }
        return false;
    }

    void NFAs_ActivityAnalyzer::mkNFA(const Task& task, NFA_fml& nfa) {
        auto initState = add_initial_state(nfa, StateProp());
        auto finalState = add_final_state(nfa, StateProp());
        for (auto activity : alphabet) {
            add_transition(nfa, initState, initState, activity);
            add_transition(nfa, finalState, finalState, activity);
        }
        auto preState = initState;
        ID i = 0;
        for (auto& activityConfig : task.getContent()) {
            if (i == task.getContent().size() - 1) {
                add_transition(nfa, preState, finalState, activityConfig.getActivity());
                break;
            }
            auto state = add_state(nfa, StateProp());
            add_transition(nfa, preState, state, activityConfig.getActivity());
            preState = state;
            i++;
        }
    }

    ID NFAs_ActivityAnalyzer::getNFAState(const string& id, Activity* activity, const Activities& lambda, NFA_fml* nfa) {
        StateProp stateProp(id, activity, lambda);
        if (stateMap.count(pair(nfa, stateProp)) == 0) {
            auto state = add_state(*nfa, stateProp);
            stateMap[pair(nfa, stateProp)] = state;
            return state;
        }
        return stateMap.at(pair(nfa, stateProp));
    }

    bool isSameNFAs(const NFAs& nfas1, const NFAs& nfas2) {
        if (nfas1.size() != nfas2.size()) return false;
        for (ID i = 0; i < nfas1.size(); i++) {
            if (nfas1.at(i).first != nfas2.at(i).first) return false;
            if (!(equal_fa(*(nfas1.at(i).second), *(nfas2.at(i).second), equal<no_type>(), StatePropEqual()))) return false;
        }
        return true;
    }

    bool NFAs_ActivityAnalyzer::addNFAs(const NFAs& newNFAs) {
        for (auto& nfas : nfaRepresentations)
            if (isSameNFAs(nfas, newNFAs)) return false;
        nfaRepresentations.emplace_back(newNFAs);
        return true;
    }

    void NFAs_ActivityAnalyzer::mkLocalNFA(ID taskID, NFA_fml* nfa) {
        auto initState = initial_state(*nfa);
        for (auto& [source, map] : transition_map(*nfa)) {
            for (auto& [activity, targets] : map)  {
                for (auto& target : targets) {
                    if (source != initState) {
                        add_transition(*nfa, initState, target, activity);
                        //auto pair = add_transition(*nfa, initState, target, activity);
                        //if (pair.second) transitionMap[nfa][activity].emplace_back(pair.first);
                    }
                }
            }
        }
        bool isChanged = true;
        while (isChanged) {
            isChanged = false;
            for (auto& [activity, targets] : transition_map(*nfa).at(initState)) {
                if (delta.count(activity) == 0) continue;
                for (auto newActivity : delta.at(activity)) {
                    auto newTaskID = getTaskID(newActivity);
                    auto lmd = newActivity -> getLaunchMode();
                    for (auto target : targets) {
                        auto& stateProp = get_property(*nfa, target);
                        if (lmd == STD || (lmd == STP && activity != newActivity)) {
                            auto state = getNFAState("q0", newActivity, stateProp.getLambda(), nfa);
                            isChanged |= add_transition(*nfa, initState, state, newActivity).second;
                            //auto pair = add_transition(*nfa, initState, state, newActivity);
                            //if (pair.second) {
                            //    isChanged = true;
                            //    transitionMap[nfa][newActivity].emplace_back(pair.first);
                            //}
                            isChanged |= add_transition(*nfa, state, target, activity).second;
                        } else if (lmd == STK && taskID == newTaskID && activity != newActivity) {
                            Activities lambda = stateProp.getLambda();
                            if (lambda.insert(newActivity).second) {
                                auto state = getNFAState("q0", newActivity, lambda, nfa);
                                isChanged |= add_transition(*nfa, initState, state, newActivity).second;
                                //auto pair = add_transition(*nfa, initState, state, newActivity);
                                //if (pair.second) {
                                //    isChanged = true;
                                //    transitionMap[nfa][newActivity].emplace_back(pair.first);
                                //}
                                isChanged |= add_transition(*nfa, state, target, activity).second;
                            }
                        }
                    }
                }
            }
        }
    }
    void NFAs_ActivityAnalyzer::delNonSourceTrans(NFA_fml* nfa, Activity* source) {
        Activities delActivities;
        auto initState = initial_state(*nfa);
        remove_transition(*nfa, initState, source);

        NFA_fml::StateSet reachableStateSet;
        reachable_closure(*nfa, reachableStateSet);
        for (auto state : state_set(*nfa))
            if (reachableStateSet.count(state) == 0) {
                clear_in_transitions(*nfa, state);
                clear_out_transitions(*nfa, state);
            }
    }

    void NFAs_ActivityAnalyzer::addSTKNFAs(const NFAs& nfas, Activity* sourceAct, Activity* targetAct) {
        NFAs newNFAs;
        for (auto& [taskID, nfa] : nfas) {
            NFA_fml* newNFA = new NFA_fml(*nfa);
            newNFAs.emplace_back(pair(taskID, newNFA));
        }
        auto topNFA = newNFAs.at(0).second;
        delNonSourceTrans(topNFA, sourceAct);
        auto newTaskID = getTaskID(targetAct);
        ID id = -1;
        for (ID i = 0; i < newNFAs.size(); i++) {
            auto& [taskID, nfa]  = newNFAs.at(i);
            if (taskID == newTaskID) {
                auto initState = initial_state(*nfa);
                for (auto& [activity, targets] : transition_map(*nfa).at(initState)) {
                    for (auto target : targets) {
                        auto& stateProp = get_property(*nfa, target);
                        auto lambda = stateProp.getLambda();
                        if (lambda.count(targetAct) == 0) {
                            lambda.insert(targetAct);
                            auto state = getNFAState("q_0", targetAct, lambda, nfa);
                            add_transition(*nfa, initState, state, targetAct);
                            add_transition(*nfa, state, target, activity);
                        }
                    }
                }
                id = i;
                break;
            }        
        }
        if (id == -1) {
            NFA_fml* nfa = new NFA_fml();
            auto initState = getNFAState("q0", nullptr, Activities(), nfa);
            auto finalState = getNFAState("qf", targetAct, Activities({targetAct}), nfa);
            set_initial_state(*nfa, initState);
            set_final_state(*nfa, finalState);
            add_transition(*nfa, initState, finalState, targetAct);
            mkLocalNFA(newTaskID, nfa);
            newNFAs.insert(newNFAs.begin(), pair(newTaskID, nfa));
            addNFAs(newNFAs);
        } else {
            auto taskID = newNFAs.at(id).first;
            auto nfa = newNFAs.at(id).second;
            mkLocalNFA(taskID, nfa);
            newNFAs.erase(newNFAs.begin() + id);
            newNFAs.insert(newNFAs.begin(), pair(taskID, nfa));
            addNFAs(newNFAs);
        }
    }

    void NFAs_ActivityAnalyzer::addPopNFAs(const NFAs& nfas) {
        for (ID i = 1; i < nfas.size(); i++) {
            NFAs newNFAs;
            for (ID j = i; j < nfas.size(); j++) {
                newNFAs.emplace_back(pair(nfas.at(j).first, new NFA_fml(*(nfas.at(j).second))));
                addNFAs(newNFAs);
            }
        }
    }


    void NFAs_ActivityAnalyzer::mkNFAs() {
        ID begin = 0, end = nfaRepresentations.size();
        while (begin < end) {
            for (ID i = begin; i < end; i++) {
                const auto& nfas = nfaRepresentations.at(i);
                auto topTaskID = nfas.at(0).first;
                auto topNFA = nfas.at(0).second;
                NFA_fml::StateSet targets;
                auto initState = initial_state(*topNFA);
                for (auto& [activity, activities] : delta) {
                    for (auto& newActivity : activities) {
                        auto newTaskID = getTaskID(newActivity);
                        if (newActivity -> getLaunchMode() == STK && newTaskID != topTaskID) {
                            targets.clear();
                            get_targets_in_map(*topNFA, initState, activity, targets);
                            if (targets.size() == 0) continue;
                            addSTKNFAs(nfaRepresentations.at(i), activity, newActivity);
                        }
                    }
                }
                addPopNFAs(nfaRepresentations.at(i));
            }
            begin = end;
            end = nfaRepresentations.size();
        }
    }        
    
}

