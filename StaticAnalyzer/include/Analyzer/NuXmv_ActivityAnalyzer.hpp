//
//  NuXmv_ActivityAnalyzer.hpp
//  TaskDroid 
//
//  Licensed under the MIT License <http://opensource.org/licenses/MIT>.
//  SPDX-License-Identifier: MIT
//  Copyright (c) 2021 Jinlong He.
//

#ifndef NuXmv_ActivityAnalyzer_hpp 
#define NuXmv_ActivityAnalyzer_hpp 

#include "ActivityAnalyzer.hpp"
#include "atl/fomula_automaton/fomula_automaton.hpp"
#include <unordered_map>
using namespace atl;
using namespace ll;
namespace TaskDroid {
    typedef unordered_map<Activity*, unordered_map<ID, std::set<ID> > > LegalPosMap;
    class NuXmv_ActivityAnalyzer : public ActivityAnalyzer {
        typedef vector<enum_value> Values;
        typedef vector<ID> Order;
        typedef unordered_map<Order, atomic_proposition > TaskPropMap;
    public:
        NuXmv_ActivityAnalyzer()
            : ActivityAnalyzer(),
              h(6),
              cmd("nuXmv"),
              nullValue("null"),
              popValue("pop"),
              isLegalPosChanged(true) {}

        NuXmv_ActivityAnalyzer(AndroidStackMachine* a, int h_)
            : ActivityAnalyzer(a),
              h(h_),
              cmd("nuXmv"),
              nullValue("null"),
              popValue("pop"),
              isLegalPosChanged(true),
              hasRTF(false) {
                  init();
              }

        NuXmv_ActivityAnalyzer(AndroidStackMachine* a, int h_, const string& cmd_)
            : ActivityAnalyzer(a),
              h(h_),
              cmd(cmd_),
              nullValue("null"),
              popValue("pop"),
              isLegalPosChanged(true),
              hasRTF(false) {
                  init();
              }
        ~NuXmv_ActivityAnalyzer() {
            for (auto item : items) {
                delete item;
                item = nullptr;
            }
        }

        bool analyzeReachability(const Configuration& config, std::ostream& os = cout);
        bool analyzeReachability(const Task& task, std::ostream& os = cout);
        const ActionMap& getDelta() const {return delta;}
        const Activities& getAlphabet() const {return alphabet;}
    private:
        bool analyzeReachability(const atomic_proposition& ap, std::ostream& os = cout);
        void init();
        void getLegalPos();
        void mkOutLegalPos();
        void getLegalPos(Activity* activity, ID taskID, ID actID);
        void getOutPorts();
        void getOutPorts(Activity* activity, ID taskID);

        void mkActivityValues();
        void mkActionValues();
        void mkOrderValues();
        void mkVars();

        atomic_proposition getTopOrderAP(ID taskID);
        atomic_proposition getTopOrderAP(ID task0ID, ID task1ID, bool eq = true);
        atomic_proposition mkIsTopActAP(ID taskID, ID actID, Activity* act);
        atomic_proposition mkIsTopNullAP(ID taskID, ID actID);
        atomic_proposition mkIsTopNonNullAP(ID taskID, ID actID);
        atomic_proposition mkIsTopTaskAP(ID taskID);
        atomic_proposition mkIsTopTaskAP(ID taskID1, ID taskID2);
        const enum_value& getSwtOrderValue(const Order& order, ID taskID, 
                                           bool finish = false);
        const enum_value& getPopOrderValue(const Order& order);
        void getSwtOrder(const Order& order, ID newTaskID, Order& newOrder);
        void getPopOrder(const Order& order, Order& newOrder);
        void getTopOrders(ID taskID, vector<Order>& newOrders);

        ID getTaskPos(const Order& order, ID taskID);
        enum_value* getMainOrderValue();
        

        // void mkADDA(ID taskID, ID actID, Activity* activity, const atomic_proposition& ap);
        void mkADDA(ID taskID, ID actID, const enum_item& item, const atomic_proposition& ap);
        void mkDELA(ID taskID, ID actID, const atomic_proposition& ap);

        void mkPOPT(ID taskID, const atomic_proposition& ap);
        void mkPOP();

        void mkPUSH(Activity* activity, Intent* intent, bool finish,
                    ID taskID, ID actID, const atomic_proposition& ap);
        void mkSTOP(Activity* activity, Intent* intent, bool finish,
                    ID taskID, ID actID, const atomic_proposition& ap);
        void mkRTOF(Activity* activity, Intent* intent, bool finish,
                    ID taskID, ID actID, const atomic_proposition& ap);
        void mkCTOP(Activity* activity, Intent* intent, bool finish,
                    ID taskID, ID actID, const atomic_proposition& ap);
        void mkCTSK(Activity* activity, Intent* intent, bool finish, 
                    ID taskID, ID actID, const atomic_proposition& ap);

        void mkSWTK(Activity* activity, Intent* intent, bool finish,
                    ID taskID, ID actID, const atomic_proposition& ap,
                    TaskPropMap& taskPropMap, bool real = true);
        void mkPUSH_N(Activity* activity, Intent* intent, bool finish,
                      ID taskID, ID actID, const atomic_proposition& ap);
        void mkSTOP_N(Activity* activity, Intent* intent, bool finish,
                      ID taskID, ID actID, const atomic_proposition& ap);
        void mkRTOF_N(Activity* activity, Intent* intent, bool finish,
                      ID taskID, ID actID, const atomic_proposition& ap);
        void mkCTOP_N(Activity* activity, Intent* intent, bool finish,
                      ID taskID, ID actID, const atomic_proposition& ap);
        void mkCTSK_N(Activity* activity, Intent* intent, bool finish,
                      ID taskID, ID actID, const atomic_proposition& ap);

        void mkSIST(Activity* activity, Intent* intent, bool finish, 
                    ID taskID, ID actID, const atomic_proposition& ap);

        void translate2FOA();

        int h;
        string cmd;
        bool isLegalPosChanged;
        LegalPosMap legalPos;
        unordered_map<ID, Activities> outPorts;
        unordered_map<ID, Activities> visited;

        bool hasRTF;
        ActionMap delta;
        Activities alphabet;
        unordered_set<ID> taskIDs;
        unordered_set<ID> sitIDSet;

        fomula_automaton<> foa;
        enum_value nullValue;
        enum_value popValue;
        enum_value mainActivityValue;
        enum_variable orderVar;
        enum_variable actionVar;
        bool_variable mainTaskVar;

        vector<item*> items;
        vector<Order> orders;
        unordered_map<ID, Values> activityValues;
        vector<enum_value> actionValues;
        vector<enum_value> orderValues;

        unordered_map<pair<ID, ID>, enum_variable*> activityVarMap;
        unordered_map<ID, enum_variable*> realActivityVarMap;

        unordered_map<Activity*, enum_value*> activityValueMap;
        unordered_map<pair<Activity*, Intent*>, enum_value*> actionValueMap;
        unordered_map<vector<ID>, enum_value*> orderValueMap;
        unordered_map<string, pair<Activity*, Action> > value2ActionMap;
    };
}
#endif /* NuXmv_ActivityAnalyzer_hpp */
