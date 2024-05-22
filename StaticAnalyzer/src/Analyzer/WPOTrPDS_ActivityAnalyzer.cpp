#include "Analyzer/WPOTrPDS_ActivityAnalyzer.hpp"
using std::cout, std::endl, std::to_string;
namespace TaskDroid {
    void WPOTrPDS_ActivityAnalyzer::init() {
        ActivityAnalyzer::init();
        Activities work({mainActivity}), newWork;
        while (work.size()) {
            for (auto activity : work) {
                if (!alphabet.insert(activity).second) continue;
                if (a -> getActionMap().count(activity) == 0) continue;
                auto& pairs = delta[activity];
                for (auto& [intent, finish] : a -> getActionMap().at(activity)) {
                    auto newActivity = intent -> getActivity();
                    auto newTaskID = getTaskID(newActivity);
                    if (newActivity -> getLaunchMode() != SIT && !finish) {
                        auto mode = AndroidStackMachine::getMode(activity, intent);
                        if (mode == PUSH || (mode == PUSH_N && newTaskID == mainTaskID)) {
                            pairs.insert(pair(newActivity, PUSH));
                            newWork.insert(newActivity);
                        } else if (mode == RTOF || (mode == RTOF_N && newTaskID == mainTaskID)) {
                            pairs.insert(pair(newActivity, RTOF));
                            newWork.insert(newActivity);
                        } else if (mode == CTOP || (mode == CTOP_N && newTaskID == mainTaskID)) {
                            pairs.insert(pair(newActivity, CTOP));
                            states.insert(newActivity);
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
        alphabet.insert(&dagActivity);
        states.insert(&p0Activity);
        util::power_set(alphabet, powAlphabet);
        mkIdTransducer();
        mkWPOTrPDS();
    }

    void WPOTrPDS_ActivityAnalyzer::mkIdTransducer() {
        unordered_map<Activity*, ID> act2dags;
        for (auto& acts : powAlphabet) {
            idTransducer.addBeta(new Beta(1, act2dags, acts));
            idTransducer.addBeta(new Beta(0, act2dags, acts));
        }
    }

    Transducer* WPOTrPDS_ActivityAnalyzer::mkActTransducer(Activity* act) {
        if (actTransducers.count(act)) return actTransducers.at(act);
        unordered_map<Activity*, ID> act2dags;
        Transducer* actTransducer = new Transducer();
        for (auto& acts : powAlphabet) {
            if (acts.count(act)) {
                actTransducer -> addBeta(new Beta(1, act2dags, acts));
                actTransducer -> addBeta(new Beta(0, act2dags, acts));
            } 
        }
        actTransducers[act] = actTransducer;
        return actTransducer;
    }

    Transducer* WPOTrPDS_ActivityAnalyzer::mkNoActTransducer(Activity* act) {
        if (noActTransducers.count(act)) return noActTransducers.at(act);
        unordered_map<Activity*, ID> act2dags;
        Transducer* noActTransducer = new Transducer();
        for (auto& acts : powAlphabet) {
            if (acts.count(act) == 0) {
                noActTransducer -> addBeta(new Beta(1, act2dags, acts));
                noActTransducer -> addBeta(new Beta(0, act2dags, acts));
            }
        }
        noActTransducers[act] = noActTransducer;
        return noActTransducer;
    }

    Transducer* WPOTrPDS_ActivityAnalyzer::mkDagTransducer(Activity* act) {
        if (dagTransducers.count(act)) return dagTransducers.at(act);
        unordered_map<Activity*, ID> act2dags;
        act2dags[act] = 1;
        Transducer* dagTransducer = new Transducer();
        for (auto& acts : powAlphabet) {
            dagTransducer -> addBeta(new Beta(1, act2dags, acts));
            dagTransducer -> addBeta(new Beta(0, act2dags, acts));
        }
        dagTransducers[act] = dagTransducer;
        return dagTransducer;
    }

    void WPOTrPDS_ActivityAnalyzer::mkWPOTrPDS() {
        pds.setStateSet(states);
        pds.setAlphabet(alphabet);
        for (auto c : alphabet) {
            pds.addTransition(&p0Activity, &p0Activity, c, vector<Activity*>(), &idTransducer);
        }
        for (auto& [source, pairs] : delta) {
            for (auto& [target, mode] : pairs) {
                if (mode == PUSH) {
                    pds.addTransition(&p0Activity, &p0Activity, source, vector<Activity*>({target, source}), &idTransducer);
                } else if (mode == RTOF) {
                    pds.addTransition(&p0Activity, &p0Activity, source, vector<Activity*>({target, source}), mkNoActTransducer(target));
                    pds.addTransition(&p0Activity, &p0Activity, source, vector<Activity*>({target, source}), mkDagTransducer(target));
                } else if (mode == CTOP) {
                    pds.addTransition(&p0Activity, &p0Activity, source, vector<Activity*>({target, source}), mkNoActTransducer(target));
                    pds.addTransition(&p0Activity, target, source, vector<Activity*>(), mkActTransducer(target));
                    pds.addTransition(target, &p0Activity, target, vector<Activity*>({target}), &idTransducer);
                    for (auto act : alphabet) {
                        if (act == target) continue;
                        pds.addTransition(target, target, act, vector<Activity*>(), &idTransducer);
                    }
                }
            }
        }
    }

    void WPOTrPDS_ActivityAnalyzer::mkWPOTrNFA(const vector<Activity*>& word) {
        auto qf = trnfa.add_final_state("qf");
        Activity* preState = &p0Activity;
        Activity* state = nullptr;
        for (ID i = 0; i < word.size(); i++) {
            if (i == word.size() - 1) state = trnfa.add_final_state("qf");
            else state = trnfa.add_state("q" + to_string(i));
            for (auto b : idTransducer.getBetas()) {
                trnfa.addTransition(preState, state, word[i], b);
                trnfa.addTransition(state, state, &dagActivity, b);
            }
            preState = state;
        }
    }

    void WPOTrPDS_ActivityAnalyzer::preStar() {
        bool flag = true;
        while (flag) {
            flag = false;
            for (auto transition : pds.getTransitions()) {
                if (transition -> target.size() == 0) {
                    for (auto& beta : transition -> t -> getBetas()) 
                    flag |= trnfa.addTransition(transition -> sourceState, transition -> sourceState, transition -> source, beta);
                    continue;
                }
                unordered_map<Activity*, unordered_set<Beta*>> targetMap;
                trnfa.getTarget(transition -> targetState, transition -> target, targetMap);
                for (auto beta : transition -> t -> getBetas()) {
                    for (auto& [t, betas] : targetMap) {
                        for (auto beta1 : betas) {
                            auto b = beta -> composition(*beta1);
                            betaSet.insert(b);
                            betaSet.insert(beta1);
                            if (b -> empty) continue;
                            flag |= trnfa.addTransition(transition -> sourceState, t, transition -> source, b);
                        }
                    }
                }
            }
        }
    }

    bool WPOTrPDS_ActivityAnalyzer::analyzeReachability(const Task& task, std::ostream& os) {
        for (auto& activityConfig : task.getContent())
            if (alphabet.count(activityConfig.getActivity()) == 0) return false;
        trnfa.clear();
        Activity* preState = &p0Activity;
        Activity* state = nullptr;
        Activity* finalState = nullptr;
        for (ID i = 0; i < task.getContent().size(); i++) {
            if (i == task.getContent().size() - 1) {
                finalState = trnfa.add_final_state("qf");
                state = finalState;
            } else {
                state = trnfa.add_state("q" + to_string(i));
            }
            for (auto b : idTransducer.getBetas()) {
                trnfa.addTransition(preState, state, task.getContent()[i].getActivity(), b);
                trnfa.addTransition(state, state, &dagActivity, b);
            }
            preState = state;
        }
        for (auto b : idTransducer.getBetas()) {
            for (auto act : alphabet) {
                trnfa.addTransition(&p0Activity, &p0Activity, act, b);
                trnfa.addTransition(finalState, finalState, act, b);
            }
        }
        preStar();
        return trnfa.accect(&p0Activity, a -> getMainActivity());
    }

    bool WPOTrPDS_ActivityAnalyzer::analyzeReachability(const Configuration& config, std::ostream& os) {
        if (config.getContent().size() > 1 || config.getContent().size() == 0) return false;
        auto& task = config.getContent().at(0);
        for (auto& activityConfig : task.getContent())
            if (alphabet.count(activityConfig.getActivity()) == 0) return false;
        trnfa.clear();
        Activity* preState = &p0Activity;
        Activity* state = nullptr;
        for (ID i = 0; i < task.getContent().size(); i++) {
            if (i == task.getContent().size() - 1) state = trnfa.add_final_state("qf");
            else state = trnfa.add_state("q" + to_string(i));
            for (auto b : idTransducer.getBetas()) {
                trnfa.addTransition(preState, state, task.getContent()[i].getActivity(), b);
                trnfa.addTransition(state, state, &dagActivity, b);
            }
            preState = state;
        }
        preStar();
        return trnfa.accect(&p0Activity, a -> getMainActivity());
    }
}


