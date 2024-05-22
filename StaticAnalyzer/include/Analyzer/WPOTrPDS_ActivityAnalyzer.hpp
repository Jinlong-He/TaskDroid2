//
//  WPOTrPDS_ActivityAnalyzer.hpp
//  TaskDroid 
//
//  Licensed under the MIT License <http://opensource.org/licenses/MIT>.
//  SPDX-License-Identifier: MIT
//  Copyright (c) 2021 Jinlong He.
//

#ifndef WPOTrPDS_ActivityAnalyzer_hpp 
#define WPOTrPDS_ActivityAnalyzer_hpp 
#include "ActivityAnalyzer.hpp"
#include <iostream>
#include <ostream>
namespace TaskDroid {
    class Beta {
    public:
        Beta(bool empty_ = true)
            : empty(empty_) {}
        Beta(bool dag_, const unordered_map<Activity*, ID>& act2dags_, const Activities& lamda_)
            : empty(false),
              dag(dag_),
              act2dags(act2dags_.begin(), act2dags_.end()),
              lamda(lamda_.begin(), lamda_.end()) {
                  for (auto& [act, n] : act2dags_) acts.insert(act);
              }
        Beta(const Beta& beta) {
            empty = beta.empty;
            dag = beta.dag;
            act2dags = beta.act2dags;
            acts = beta.acts;
            lamda = beta.lamda;
        }
        bool operator== (const Beta& beta) const {
            return (*this <= beta) && (beta <= *this);
        }

        bool operator!= (const Beta& beta) const {
            return !(*this == beta);
        }

        bool operator<= (const Beta& beta) const {
            if (empty) return true;
            if (dag != beta.dag || lamda != beta.lamda) return false;
            if (act2dags.size() == 0 && act2dags.size() == 0) return true;
            if (acts == beta.acts) {
                for (auto& [act, n] : act2dags) {
                    if (n < beta.act2dags.at(act)) return false;
                }
                return true;
            }
            return false;
        }

        friend std::ostream& operator<< (std::ostream& os, const Beta& beta) {
            if (beta.empty) {
                os << "empth";
            } else {
                if (beta.dag) os << "dag, ";
                else os << "nodag, ";
                for (auto& [act, n] : beta.act2dags) {
                    os << act -> getName() << ": " << n << ", ";
                }
                os << "; ";
                for (auto act : beta.lamda) os << act -> getName() << ",";
            }
            return os;
        }

        bool isDag(Activity* activity) const {
            return activity -> getName() == "dag";
        }

        Beta* composition(const Beta& beta) const {
            if (empty || beta.empty) return new Beta();
            if ((dag || act2dags.size() > 0) && !beta.dag) return new Beta();
            Activities lamda1 = beta.lamda;
            lamda1.insert(beta.acts.begin(), beta.acts.end());
            if (lamda != lamda1) return new Beta();
            Beta* b = new Beta(*this);
            for (auto& [act, n] : beta.act2dags) {
                if (b -> acts.insert(act).second) {
                    b -> act2dags[act] = n;
                } else {
                    b -> act2dags[act] = b -> act2dags[act] + n;
                }
            }
            b -> lamda = beta.lamda;
            return b;
        }

        vector<Beta*> leftQuotient(Activity* upper, Activity* lower) const {
            if (acts.count(upper) > 0 && isDag(lower) && act2dags.at(upper) > 1) {
                Beta* b = new Beta(*this);
                b -> act2dags[upper] = b -> act2dags[upper] - 1;
                return vector<Beta*>({b});
            }
            if (acts.count(upper) > 0 && isDag(lower) && act2dags.at(upper) == 1) {
                Beta* b1 = new Beta(*this);
                Beta* b2 = new Beta(*this);
                b1 -> acts.erase(upper);
                b1 -> act2dags.erase(upper);
                return vector<Beta*>({b1, b2});
            }
            if (upper == lower && lamda.count(upper) > 0 && acts.count(upper) == 0) {
                Beta* b = new Beta(*this);
                return vector<Beta*>({b});
            }
            return vector<Beta*>();
        }

        vector<Beta*> union_(const Beta& beta) {
            if (*this <= beta) return vector<Beta*>({new Beta(beta)});
            if (beta <= *this) return vector<Beta*>({new Beta(*this)});
            return vector<Beta*>({new Beta(*this), new Beta(beta)});
        }

        bool empty;
        bool dag;
        unordered_map<Activity*, ID> act2dags;
        Activities acts;
        Activities lamda;
    };

    class Transducer {
    public:
        Transducer() {}
        Transducer(const vector<Beta*>& betas_)
            : betas(betas_) {}
        ~Transducer() {
            for (auto beta : betas) {
                delete beta;
                beta = nullptr;
            }
        }
        void addBeta(Beta* beta) {
            betas.emplace_back(beta);
        }
        const vector<Beta*>& getBetas() const {
            return betas;
        }
        friend std::ostream& operator<< (std::ostream& os, const Transducer& t) {
            for (auto beta : t.betas) {
                os << *beta << endl;
            }
            return os;
        }
    private:
        vector<Beta*> betas;
    };

    struct WPOTrPDSTrans {
        WPOTrPDSTrans(Activity* s, Activity* t, Activity* source_, const vector<Activity*>& target_, Transducer* t_)
            : sourceState(s),
              targetState(t),
              source(source_),
              target(target_.begin(), target_.end()),
              t(t_) {}
        Activity* sourceState;
        Activity* targetState;
        Activity* source;
        vector<Activity*> target;
        Transducer* t;
    };

    class WPOTrPDS {
    public:
        WPOTrPDS() {}
        ~WPOTrPDS() {
            for (auto t : transitions) {
                delete t;
                t = nullptr;
            }
        }
        void setStateSet(const Activities& states) {
            stateSet = states;
        }
        void setAlphabet(const Activities& states) {
            alphabet = states;
        }
        void addTransition(Activity* s, Activity* t, Activity* source, const vector<Activity*>& target, Transducer* transducer) {
            transitions.emplace_back(new WPOTrPDSTrans(s, t, source, target, transducer));
        }
        const vector<WPOTrPDSTrans*>& getTransitions() const {
            return transitions;
        }
        void print() const {
            for (auto transition : transitions) {
                cout << transition -> sourceState -> getName() << " -> " << transition -> targetState -> getName() << ": ";
                if (transition -> target.size() == 0)
                    cout << transition -> source -> getName() << ", <>";
                else if (transition -> target.size() == 2)
                    cout << transition -> source -> getName() << ", <" << transition -> target[0] -> getName() << ", " << transition -> target[1] -> getName() << ">";
                else
                    cout << transition -> source -> getName() << ", <" << transition -> target[0] -> getName() << ">";
                cout << "; " << transition -> t << endl;
            }
        }
    private:
        Activities stateSet;
        Activities alphabet;
        vector<WPOTrPDSTrans*> transitions;
    };

    class WPOTrNFA {
    public:
        WPOTrNFA() {}
        WPOTrNFA(const Activities& states)
            : controlStateSet(states) {}
        ~WPOTrNFA() {
            for (auto state : stateSet) {
                delete state;
                state = nullptr;
            }
        }
        void clear() {
            stateSet.clear();
            controlStateSet.clear();
            finalStateSet.clear();
            transitions.clear();
        }
        Activity* add_state(const string& name) {
            Activity* state = new Activity(name, "", STD);
            stateSet.insert(state);
            return state;
        }
        Activity* add_final_state(const string& name) {
            auto state = add_state(name);
            finalStateSet.insert(state);
            return state;
        }
        bool addTransition(Activity* s, Activity* t, Activity* c, Beta* b) {
            if (transitions.count(s) == 0 || transitions.at(s).count(c) == 0 ||
                transitions.at(s).at(c).count(t) == 0) {
                transitions[s][c][t].insert(b);
                return true;
            } else {
                auto& betas = transitions[s][c][t];
                for (auto beta : betas)
                    if (*b <= *beta) return false;
                betas.insert(b);
                return true;
            }
            return false;
        }
        void getTarget(Activity* source, const vector<Activity*>& word, unordered_map<Activity*, unordered_set<Beta*> >& targetMap) {
            if (word.size() == 2) {
                auto a1 = word[0];
                auto a2 = word[1];
                if (transitions.count(source) == 0 || transitions.at(source).count(a1) == 0) return;
                for (auto& [mid, betas] : transitions.at(source).at(a1)) {
                    if (transitions.count(mid) == 0) continue;
                    for (auto& [a, map] : transitions.at(mid)) {
                        for (auto beta : betas) {
                            vector<Beta*> lBetas = beta -> leftQuotient(a2, a);
                            if (lBetas.size() == 0) continue;
                            for (auto lBeta : lBetas) {
                                for (auto& [target, newBetas] : map) {
                                    for (auto nBeta : newBetas) {
                                        auto resBeta = lBeta -> composition(*nBeta);
                                        if (resBeta -> empty) continue;
                                        targetMap[target].insert(resBeta);
                                    }
                                }
                            }
                            for (auto lBeta : lBetas) {
                                delete lBeta;
                                lBeta = nullptr;
                            }
                        }
                    }
                }
            } else if (word.size() == 1){
                auto a1 = word[0];
                if (transitions.count(source) == 0 || transitions.at(source).count(a1) == 0) return;
                for (auto& [target, betas] : transitions.at(source).at(a1)) {
                    for (auto beta : betas)
                        targetMap[target].insert(new Beta(*beta));
                }
            }
        }
        void print() const {
            for (auto& [source, map1] : transitions) {
                for (auto& [symbol, map2] : map1) {
                    for (auto& [target, betas] :map2) {
                        for (auto beta : betas) {
                            cout << source -> getName() << " -> " << target -> getName() << ": " << symbol -> getName() << ", " << *beta << endl;
                        }
                    }
                }
            }
        }
        bool accect(Activity* s, Activity* c) {
            if (transitions.count(s) == 0 || transitions.at(s).count(c) == 0) return false;
            for (auto t : finalStateSet)
                if (transitions.at(s).at(c).count(t)) return true;
            return false;
        }
    private:
        Activities stateSet;
        Activities controlStateSet;
        Activities finalStateSet;
        unordered_map<Activity*, unordered_map<Activity*, unordered_map<Activity*, unordered_set<Beta*> > > > transitions;
    };

    class WPOTrPDS_ActivityAnalyzer : public ActivityAnalyzer {
    public:
        WPOTrPDS_ActivityAnalyzer()
            : ActivityAnalyzer(),
              p0Activity("p0", "", STD),
              dagActivity("dag", "", STD),
              trnfa(states) {}

        WPOTrPDS_ActivityAnalyzer(AndroidStackMachine* a)
            : ActivityAnalyzer(a),
              p0Activity("p0", "", STD),
              dagActivity("dag", "", STD),
              trnfa(states) {
                init();
            }

        ~WPOTrPDS_ActivityAnalyzer() {
            for (auto beta : betaSet) {
                delete beta;
                beta = nullptr;
            }
            for (auto& [act, transducer] : actTransducers) {
                delete transducer;
                transducer = nullptr;
            }
            for (auto& [act, transducer] : noActTransducers) {
                delete transducer;
                transducer = nullptr;
            }
            for (auto& [act, transducer] : dagTransducers) {
                delete transducer;
                transducer = nullptr;
            }
        }

        bool analyzeReachability(const Configuration& config, std::ostream& os = std::cout);
        bool analyzeReachability(const Task& task, std::ostream& os = std::cout);
        bool isSingleTask() const;
        const Activities& getAlphabet() const { return alphabet; }
        const unordered_map<Activity*, unordered_set<pair<Activity*, ActionMode> > >& getDelta() {return delta;}
    private:
        void init();
        void mkWPOTrPDS();
        void mkWPOTrNFA(const vector<Activity*>& word);
        void preStar();
        void mkIdTransducer();
        Transducer* mkActTransducer(Activity* act);
        Transducer* mkNoActTransducer(Activity* act);
        Transducer* mkDagTransducer(Activity* act);
        Activity p0Activity;
        Activity dagActivity;
        Activities alphabet;
        Activities states;
        unordered_set<Activities> powAlphabet;
        unordered_map<Activity*, unordered_set<pair<Activity*, ActionMode> > > delta;
        unordered_set<Beta*> betaSet;
        Transducer idTransducer;
        unordered_map<Activity*, Transducer*> noActTransducers;
        unordered_map<Activity*, Transducer*> actTransducers;
        unordered_map<Activity*, Transducer*> dagTransducers;
        WPOTrPDS pds;
        WPOTrNFA trnfa;
    };
}

#endif /* WPOTrPDS_ActivityAnalyzer_hpp */


