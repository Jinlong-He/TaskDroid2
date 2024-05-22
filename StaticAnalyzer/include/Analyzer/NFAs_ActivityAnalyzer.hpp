//
//  NFAs_ActivityAnalyzer.hpp
//  TaskDroid 
//
//  Licensed under the MIT License <http://opensource.org/licenses/MIT>.
//  SPDX-License-Identifier: MIT
//  Copyright (c) 2021 Jinlong He.
//

#ifndef NFAs_ActivityAnalyzer_hpp 
#define NFAs_ActivityAnalyzer_hpp 
#include "ActivityAnalyzer.hpp"
#include "atl/finite_automaton/nondeterministic_finite_automaton.hpp"
#include "atl/finite_automaton/deterministic_finite_automaton.hpp"
#include <iostream>
#include <ostream>
namespace TaskDroid {
    class StateProp {
    public:
        StateProp()
            : id(""),
              activity(nullptr) {}

        StateProp(const string& id_, Activity* activity_)
            : id(id_),
              activity(activity_) {}

        StateProp(const string& id_, Activity* activity_, const Activities& lambda_)
            : id(id_),
              activity(activity_),
              lambda(lambda_) {}

        const string& getID() const {
            return id;
        }

        Activity* getActivity() const {
            return activity;
        }

        const Activities& getLambda() const {
            return lambda;
        }

        bool operator== (const StateProp& state) const {
            return id == state.id && activity == state.activity && lambda == state.lambda;
        }

        bool operator!= (const StateProp& state) const {
            return !(*this == state);
        }

        StateProp operator| (const StateProp& state) const {
            return StateProp("", activity, lambda);
        }

        StateProp operator& (const StateProp& state) const {
            return StateProp("", activity, lambda);
        }

        friend std::ostream& operator<< (std::ostream& os, const StateProp& state) {
            if (state.activity != nullptr)
                os << state.id << " " << state.activity -> getName() << " " << state.lambda.size();
            else 
                os << state.id << " null " << state.lambda.size();
            return os;
        }

        private:
            string id;
            Activity* activity;
            Activities lambda;
    };

    struct StatePropEqual {
        bool operator() (const StateProp& lhs, const StateProp& rhs) const {
            return true;
        }
    };
}

namespace std {
    template<>
    struct hash<TaskDroid::StateProp> {
        std::size_t operator() (const TaskDroid::StateProp& s) const{
            return boost::hash<string>()(s.getID()) ^ 
                   boost::hash_value(s.getActivity()) ^
                   hash<unordered_set<TaskDroid::Activity*> >()(s.getLambda());
        }
    };
}

namespace TaskDroid {
    typedef atl::nondeterministic_finite_automaton<Activity*, atl::no_type, StateProp> NFA_fml;
    typedef atl::deterministic_finite_automaton<Activity*> DFA_fml;
    typedef vector<pair<ID, NFA_fml*> > NFAs;
    class NFAs_ActivityAnalyzer : public ActivityAnalyzer {
    public:
        NFAs_ActivityAnalyzer()
            : ActivityAnalyzer() {}

        NFAs_ActivityAnalyzer(AndroidStackMachine* a)
            : ActivityAnalyzer(a) {
                init();
            }

        ~NFAs_ActivityAnalyzer() {
            for (auto& nfas : nfaRepresentations) {
                for (auto& [id, nfa] : nfas) {
                    delete nfa;
                    nfa = nullptr;
                }
            }
        }

        bool analyzeReachability(const Configuration& config, std::ostream& os = std::cout);
        bool analyzeReachability(const Task& task, std::ostream& os = std::cout);
        bool isSITFree() const;
        void mkNFAs();
        const Activities& getAlphabet() const { return alphabet; }
        const unordered_map<Activity*, Activities>& getDelta() const { return delta; }
    private:
        void init();
        void mkLocalNFA(ID taskID, NFA_fml* nfa);
        bool addNFAs(const NFAs& nfas);
        void addSTKNFAs(const NFAs& nfas, Activity* source, Activity* target);
        void addPopNFAs(const NFAs& nfas);
        void delNonSourceTrans(NFA_fml* nfa, Activity* source);
        ID getNFAState(const string& id, Activity* activity, const Activities& lambda, NFA_fml* nfa);
        void mkNFA(const Task& task, NFA_fml& nfa);

        vector<NFAs> nfaRepresentations;
        unordered_map<pair<NFA_fml*, StateProp>, ID> stateMap;
        unordered_map<Activity*, Activities> delta;
        unordered_map<NFA_fml*, unordered_map<Activity*, vector<NFA_fml::Transition> > > transitionMap;
        Activities alphabet;
    };
}

#endif /* NFAs_ActivityAnalyzer_hpp */


