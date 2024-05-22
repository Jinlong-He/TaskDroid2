#include "Analyzer/NuXmv_ActivityAnalyzer.hpp"
#include <ostream>
using std::cout, std::endl, std::to_string;
namespace TaskDroid {
    void NuXmv_ActivityAnalyzer::init() {
        if (a -> getActionMap().size() == 0) return;
        ActivityAnalyzer::init();
        for (auto& [activity, actions] : a -> getActionMap()) {
            alphabet.insert(activity);
            for (auto& [intent, finish] : actions) {
                auto newActivity = intent -> getActivity();
                auto mode = AndroidStackMachine::getMode(activity, intent);
                if (mode == MKTK) continue;
                if (mode == RTOF || mode == RTOF_N) hasRTF = true;
                alphabet.insert(newActivity);
                delta[activity].insert(pair(intent, finish));
                if (activity -> getLaunchMode() == SIT) sitIDSet.insert(getTaskID(activity));
                if (newActivity -> getLaunchMode() == SIT) sitIDSet.insert(getTaskID(newActivity));
            }
        }
        getLegalPos();
        mkActivityValues();
        mkActionValues();
        mkOrderValues();
        mkVars();
        translate2FOA();
    }

    bool NuXmv_ActivityAnalyzer::analyzeReachability(const Configuration& config, std::ostream& os) {
        vector<Activity*> word;
        vector<pair<Activity*, vector<Activity*> > > words;
        for (auto& task : config.getContent()) {
            if (task.getContent().size() > h) return false;
            word.clear();
            for (auto& ac : task.getContent()) {
                auto activity = ac.getActivity();
                if (alphabet.count(activity) == 0) return false;
                word.emplace_back(activity);
            }
            if (alphabet.count(task.getRealActivity()) == 0) return false;
            words.emplace_back(pair(task.getRealActivity(), word));
        }
        Order order(taskNum, -1);
        auto ap = atomic_proposition("TRUE");
        ID id = 0;
        for (auto& [realAct, word] : words) {
            auto& var = *(realActivityVarMap.at(getTaskID(realAct)));
            auto taskID = getTaskID(realAct);
            auto realAP = var == *(activityValueMap.at(realAct));
            auto p = atomic_proposition("TRUE");
            order[taskID] = id++;
            for (ID i = 0; i < word.size(); i++) {
                auto& var = *activityVarMap.at(pair(taskID, i));
                auto& value = *activityValueMap.at(word[word.size() - i - 1]);
                p = p & var == value;
            }
            if (word.size() < h) {
                auto& var = *activityVarMap.at(pair(taskID, word.size()));
                ap = ap & realAP & p & var == nullValue;
            } else ap = ap & realAP & p;
        }
        ap = ap & orderVar == *(orderValueMap.at(order));
        return analyzeReachability(ap, os);
    }

    bool NuXmv_ActivityAnalyzer::analyzeReachability(const Task& task, std::ostream& os) {
        auto ap = atomic_proposition("FALSE");
        auto& content = task.getContent();
        if (content.size() > h || content.size() == 0) return false;
        auto taskID = getTaskID(task.getRealActivity());
        for (ID i = 0; i <= h - content.size(); i++) {
            atomic_proposition p("TRUE");
            for (ID j = i; j - i < content.size(); j++) {
                auto& var = *activityVarMap.at(pair(taskID, j));
                auto& value = *activityValueMap.at(content[content.size() - j + i -1].getActivity());
                p = p & var == value;
            }
            ap = ap | p;
        }
        return analyzeReachability(ap, os);
    }

    void NuXmv_ActivityAnalyzer::getOutPorts(Activity* activity, ID taskID) {
        if (!visited[taskID].insert(activity).second) return;
        if (a -> getActionMap().count(activity) == 0) return;
        for (auto& [intent, finish] : a -> getActionMap().at(activity)) {
            auto newActivity = intent -> getActivity();
            auto newTaskID = getTaskID(newActivity);
            if (AndroidStackMachine::isNewMode(activity, intent) && taskID != newTaskID) {
                outPorts[taskID].insert(activity);
                getLegalPos(newActivity, newTaskID, 0);
                getOutPorts(newActivity, newTaskID);
            } else {
                getOutPorts(newActivity, taskID);
            }
        }
    }

    void NuXmv_ActivityAnalyzer::getLegalPos(Activity* activity, ID taskID, ID actID) {
        if (actID > h - 1 || actID < 0) return;
        if (legalPos[activity][taskID].insert(actID).second) {
            isLegalPosChanged = true;
            if (a -> getActionMap().count(activity) == 0) return;
            for (auto& [intent, finish] : a -> getActionMap().at(activity)) {
                auto newActivity = intent -> getActivity();
                auto newTaskID = getTaskID(newActivity);
                auto mode = AndroidStackMachine::getMode(activity, intent);
                if (mode == PUSH || (mode == PUSH_N &&  newTaskID == taskID) ||
                    (activity != newActivity && (mode == CTOP || (mode == CTOP_N && newTaskID == taskID))) ||
                    (activity != newActivity && (mode == STOP || (mode == STOP_N && newTaskID == taskID)))) {
                    if (finish) getLegalPos(newActivity, taskID, actID);
                     else getLegalPos(newActivity, taskID, actID + 1);
                } else if (mode == CTSK_N && newTaskID == taskID) {
                    getLegalPos(newActivity, taskID, 0);
                } 
            }
        }
    }

    void NuXmv_ActivityAnalyzer::mkOutLegalPos() {
        while (isLegalPosChanged) {
            isLegalPosChanged = false;
            for (auto& [taskID, outs] : outPorts) {
                for (auto out : outs) {
                    if (a -> getActionMap().count(out) == 0) return;
                    for (auto& [intent, finish] : a -> getActionMap().at(out)) {
                        auto newActivity = intent -> getActivity();
                        auto newTaskID = getTaskID(newActivity);
                        auto mode = AndroidStackMachine::getMode(out, intent);
                        if (AndroidStackMachine::isNewMode(out, intent) && taskID != newTaskID) {
                            if (outPorts.count(newTaskID) == 0) continue;
                            for (auto newOut : outPorts.at(newTaskID)) {
                                for (auto newActID : legalPos.at(newOut).at(newTaskID)) {
                                    if (mode == PUSH_N || 
                                        (mode == STOP_N && newActivity != newOut) ||
                                        (mode == CTOP_N && newActivity != newOut)) {
                                        getLegalPos(newActivity, newTaskID, newActID + 1); 
                                        getLegalPos(newActivity, newTaskID, newActID); 
                                    } 
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    void NuXmv_ActivityAnalyzer::getLegalPos() {
        getOutPorts(mainActivity, mainTaskID);
        if (hasRTF) {
            std::set<ID> ids;
            for (ID i = 0; i < h; i++)
                ids.insert(i);
            for (auto& [id, acts] : visited) {
                for (auto act : acts) {
                    if (act -> getLaunchMode() == SIT) legalPos[act][id] = std::set<ID>({0});
                    else legalPos[act][id] = ids;
                }
            }
            return;
        }
        getLegalPos(mainActivity, mainTaskID, 0);
        mkOutLegalPos();
    }

    template<class T>
    void getOrders(size_t num, T null, vector<vector<T> >& orders) {
        vector<vector<T> > com;
        vector<T> vec, datas;
        for (size_t i = 0; i < num; i++) datas.emplace_back(i);
        for (size_t i = 1; i <= datas.size(); i++) {
            vec.clear();
            for (size_t j = 0; j < datas.size(); j++) {
                if (j < i) vec.emplace_back(datas[j]);
                else vec.emplace_back(null);
            }
            sort(vec.begin(), vec.end());
            com.emplace_back(vec);
        }
        for (size_t i = 0; i < com.size(); i++) {
            do {
                vector<T> n;
                for (size_t j = 0; j < com[i].size(); j++) {
                    n.emplace_back(com[i][j]);
                }
                orders.emplace_back(n);
            }
            while (next_permutation(com[i].begin(), com[i].end()));
        }
        orders.emplace_back(vector<T>(num, null));
    }

    enum_value* NuXmv_ActivityAnalyzer::getMainOrderValue() {
        Order mainOrder;
        for (auto i = 0; i < taskNum; i++) {
            if (i == mainTaskID) mainOrder.emplace_back(0);
            else mainOrder.emplace_back(-1);
        }
        return orderValueMap.at(mainOrder);
    }

    void NuXmv_ActivityAnalyzer::mkActivityValues() {
        ID id = 0;
        for (auto activity : alphabet) {
            enum_value* v = new enum_value("a" + to_string(id++));
            activityValueMap[activity] = v;
            items.emplace_back(v);
        }
        activityValueMap[nullptr] = &nullValue;
        mainActivityValue = *activityValueMap.at(a -> getMainActivity());
        for (auto& [id, acts] : visited) {
            auto& values = activityValues[id];
            values.emplace_back(nullValue);
            for (auto activity : acts)
                values.emplace_back(*activityValueMap.at(activity));
        }
    }

    void NuXmv_ActivityAnalyzer::mkActionValues() {
        ID id = 0;
        for (auto& [activity, actions] : a -> getActionMap()) {
            for (auto& [intent, finish]  : actions) {
                string name = "t" + to_string(id++);
                enum_value* v = new enum_value(name);
                actionValues.emplace_back(*v);
                actionValueMap[pair(activity, intent)] = v;
                items.emplace_back(v);
                value2ActionMap[name] = pair(activity, Action(intent, finish));
            }
        }
        actionValues.emplace_back(popValue);
    }

    void NuXmv_ActivityAnalyzer::mkOrderValues() {
        getOrders(taskNum, (ID)-1, orders);
        for (auto& order : orders) {
            string name = "o";
            for (auto i : order) {
                if (i != -1) name += "_" + to_string(i);
                else name += "_null";
            }
            // cout << name << endl;
            enum_value* v = new enum_value(name);
            orderValueMap[order] = v;
            orderValues.emplace_back(*v);
            items.emplace_back(v);
        }
    }

    void NuXmv_ActivityAnalyzer::mkVars() {
        for (ID i = 0; i < taskNum; i++) {
            ID hh = h;
            if (sitIDSet.count(i) > 0) hh = 1;
            for (ID j = 0; j < hh; j++) {
                string name = "a_" + to_string(i) + "_" + to_string(j);
                enum_variable* v = new enum_variable(name, activityValues.at(i).begin(), activityValues.at(i).end());
                if (i == mainTaskID && j == 0) add_control_state(foa, *v, (*v == mainActivityValue));
                else add_control_state(foa, *v, (*v == nullValue));
                activityVarMap[pair(i,j)] = v;
                items.emplace_back(v);
            }
            string name = "r_" + to_string(i);
            const auto& acts = realActivities.at(i);
            vector<enum_value> values({nullValue});
            for (auto act : acts)
                values.emplace_back(*activityValueMap.at(act));
            enum_variable* v = new enum_variable(name, values.begin(), values.end());
            if (i == mainTaskID) add_control_state(foa, *v, (*v == mainActivityValue));
            else add_control_state(foa, *v, (*v == nullValue));
            realActivityVarMap[i] = v;
            items.emplace_back(v);
        }
        actionVar = enum_variable("t", actionValues.begin(), actionValues.end());
        add_input_state(foa, actionVar);
        mainTaskVar = bool_variable("mb");
        add_control_state(foa, mainTaskVar, mainTaskVar == bool_value(1));
        orderVar = enum_variable("o", orderValues.begin(), orderValues.end());
        add_control_state(foa, orderVar, orderVar == *getMainOrderValue());
    }

    atomic_proposition NuXmv_ActivityAnalyzer::getTopOrderAP(ID taskID) {
        atomic_proposition ap("FALSE");
        for (auto& [order, v] : orderValueMap) {
            if (order[taskID] == 0) ap = ap | orderVar == *v;
        }
        if (ap.to_string() == "FALSE") return atomic_proposition("TRUE");
        return ap;
    }

    atomic_proposition NuXmv_ActivityAnalyzer::getTopOrderAP(ID task0ID, ID task1ID, bool eq) {
        atomic_proposition ap("FALSE");
        for (auto& [order, v] : orderValueMap) {
            if (eq && order[task0ID] == 0 && order[task1ID] == 1) 
                ap = ap | orderVar == *v;
            if (!eq && order[task0ID] == 0 && order[task1ID] >= 1) 
                ap = ap | orderVar == *v;
        }
        if (ap.to_string() == "FALSE") return atomic_proposition("TRUE");
        return ap;
    }

    const enum_value& NuXmv_ActivityAnalyzer::getSwtOrderValue(const Order& order, 
                                                               ID newTaskID, 
                                                               bool finish) {
        Order newOrder;
        if (finish) {
            Order popOrder;
            getPopOrder(order, popOrder);
            getSwtOrder(popOrder, newTaskID, newOrder);
        } else {
            getSwtOrder(order, newTaskID, newOrder);
        }
        return *orderValueMap.at(newOrder);
    }

    const enum_value& NuXmv_ActivityAnalyzer::getPopOrderValue(const Order& order) {
        Order popOrder;
        getPopOrder(order, popOrder);
        return *orderValueMap.at(popOrder);
    }

    void NuXmv_ActivityAnalyzer::getSwtOrder(const Order& order, ID newTaskID,
                                        Order& newOrder) {
        newOrder = order;
        newOrder[newTaskID] = 0;
        if (order[newTaskID] == -1) {
            for (ID i = 0; i < order.size(); i++) {
                if (order.at(i) != -1) newOrder[i] = order.at(i) + 1;
            }
        } else {
            auto o = order[newTaskID];
            for (ID i = 0; i < order.size(); i++) {
                if (order.at(i) < o) newOrder[i] = order.at(i) + 1;
            }
        }
    }

    void NuXmv_ActivityAnalyzer::getPopOrder(const Order& order, Order& newOrder) {
        newOrder.clear();
        for (auto id : order) 
            newOrder.emplace_back(id == -1 ? -1 : id - 1);
    }

    void NuXmv_ActivityAnalyzer::getTopOrders(ID taskID, vector<Order>& newOrders) {
        for (auto& order : orders)
            if (order.at(taskID) == 0) newOrders.emplace_back(order);
    }

    atomic_proposition NuXmv_ActivityAnalyzer::mkIsTopNullAP(ID taskID, ID actID) {
        auto& var = *activityVarMap.at(pair(taskID, actID));
        if (actID == 0) {
            return (var == nullValue);
        } else {
            auto& actVar = *activityVarMap.at(pair(taskID, actID - 1));
            return (var == nullValue & actVar != nullValue);
        }
    }

    atomic_proposition NuXmv_ActivityAnalyzer::mkIsTopNonNullAP(ID taskID, ID actID) {
        auto& var = *activityVarMap.at(pair(taskID, actID));
        if (sitIDSet.count(taskID) > 0) return var != nullValue;
        if (actID == h - 1) {
            return (var != nullValue);
        } else {
            auto& actVar = *activityVarMap.at(pair(taskID, actID + 1));
            return (var != nullValue & actVar == nullValue);
        }
    }

    atomic_proposition NuXmv_ActivityAnalyzer::mkIsTopActAP(ID taskID, ID actID, Activity* activity) {
        auto& var = *activityVarMap.at(pair(taskID, actID));
        auto& value = *activityValueMap.at(activity);
        if (sitIDSet.count(taskID) > 0) return var == value;
        if (actID == h - 1) {
            return (var == value);
        } else {
            auto& nullVar = *activityVarMap.at(pair(taskID, actID + 1));
            return (var == value & nullVar == nullValue);
        }
    }

    atomic_proposition NuXmv_ActivityAnalyzer::mkIsTopTaskAP(ID taskID) {
        atomic_proposition ap("FALSE");
        for (auto& [order, v] : orderValueMap) {
            if (order[taskID] == 0) ap = ap | orderVar == *v;
        }
        if (taskNum > 1) return ap;
        return atomic_proposition("TRUE");
    }

    atomic_proposition NuXmv_ActivityAnalyzer::mkIsTopTaskAP(ID taskID1, ID taskID2) {
        atomic_proposition ap("FALSE");
        for (auto& [order, v] : orderValueMap) {
            if (order[taskID1] == 0 && order[taskID2] != -1) ap = ap | orderVar == *v;
        }
        if (taskNum > 1) return ap;
        return atomic_proposition("TRUE");
    }

    void NuXmv_ActivityAnalyzer::mkADDA(ID taskID, ID actID, const enum_item& item,
                                        const atomic_proposition& ap) {
        auto& var = *activityVarMap.at(pair(taskID, actID));
        // auto& value = *activityValueMap.at(activity);
        add_transition(foa, var, item, ap);
    }

    void NuXmv_ActivityAnalyzer::mkDELA(ID taskID, ID actID,
                                        const atomic_proposition& ap) {
        auto& var = *activityVarMap.at(pair(taskID, actID));
        add_transition(foa, var, nullValue, ap);
    }

    void NuXmv_ActivityAnalyzer::mkPOPT(ID taskID, const atomic_proposition& ap) {
        vector<Order> topOrders;
        getTopOrders(taskID, topOrders);
        for (auto& order : topOrders) {
            Order popOrder;
            getPopOrder(order, popOrder);
            auto& value = *orderValueMap.at(order);
            auto& newValue = getPopOrderValue(order);
            add_transition(foa, orderVar, newValue, ap & (orderVar == value));
        }
    }

    void NuXmv_ActivityAnalyzer::mkPUSH(Activity* activity, Intent* intent, 
                                        bool finish, ID taskID, ID actID,
                                        const atomic_proposition& ap) {
        auto orderAP = mkIsTopTaskAP(taskID);
        auto newActivity = intent -> getActivity();
        auto& newValue = *(activityValueMap.at(newActivity));
        if (!finish) {
            if (actID < h - 1)
                mkADDA(taskID, actID + 1, newValue, ap & orderAP);
        } else {
            mkADDA(taskID, actID, newValue, ap & orderAP);
        }
    }

    void NuXmv_ActivityAnalyzer::mkSTOP(Activity* activity, Intent* intent, 
                                        bool finish, ID taskID, ID actID,
                                        const atomic_proposition& ap) {
        auto orderAP = mkIsTopTaskAP(taskID);
        auto newActivity = intent -> getActivity();
        auto& newValue = *(activityValueMap.at(newActivity));
        if (!finish) {
            if (activity == newActivity) return;
            mkPUSH(activity, intent, finish, taskID, actID, ap);
        } else {
            if (activity == newActivity) {
                mkDELA(taskID, actID, ap & orderAP);
                if (actID == 0) mkPOPT(taskID, ap);
            } else {
                mkADDA(taskID, actID, newValue, ap & orderAP);
            }
        }
    }

    void NuXmv_ActivityAnalyzer::mkRTOF(Activity* activity, Intent* intent, 
                                        bool finish, ID taskID, ID actID,
                                        const atomic_proposition& ap) {
        auto orderAP = mkIsTopTaskAP(taskID);
        auto& value = *activityValueMap.at(activity);
        auto newActivity = intent -> getActivity();
        auto& newValue = *activityValueMap.at(newActivity);
        if (activity == newActivity) {
            if (finish) {
                mkDELA(taskID, actID, ap & orderAP);
                if (actID == 0) mkPOPT(taskID, ap);
            } 
            else return;
        } else {
            if (legalPos.count(newActivity) == 0 ||
                legalPos.at(newActivity).count(taskID) == 0) return;
            auto& set = legalPos.at(newActivity).at(taskID);
            vector<ID> poses(set.begin(), set.end());
            auto noAP = atomic_proposition("TRUE");
            for (ID i = poses.size() - 1; i != -1; i--) {
                if (poses.at(i) >= actID) continue;
                auto& var = *activityVarMap.at(pair(taskID, poses.at(i)));
                auto p = (var == newValue);
                mkADDA(taskID, actID, newValue, ap & orderAP & noAP & p);
                for (ID j = poses.at(i); j < actID; j++) {
                    auto& var = *activityVarMap.at(pair(taskID, j + 1));
                    mkADDA(taskID, j, var, ap & orderAP & noAP & p);
                }
                noAP = noAP & (var != newValue);
            }
            if (!finish && actID < h - 1)
                mkADDA(taskID, actID + 1, newValue, ap & orderAP & noAP);
            if (finish) 
                mkADDA(taskID, actID, newValue, ap & orderAP & noAP);
        }
    }

    void NuXmv_ActivityAnalyzer::mkCTOP(Activity* activity, Intent* intent, 
                                        bool finish, ID taskID, ID actID,
                                        const atomic_proposition& ap) {
        auto orderAP = mkIsTopTaskAP(taskID);
        auto newActivity = intent -> getActivity();
        auto& newValue = *activityValueMap.at(newActivity);
        if (activity == newActivity) {
            if (finish) {
                mkDELA(taskID, actID, ap & orderAP);
                if (actID == 0) mkPOPT(taskID, ap);
            } 
        } else {
            if (legalPos.count(newActivity) == 0 ||
                legalPos.at(newActivity).count(taskID) == 0) return;
            auto& set = legalPos.at(newActivity).at(taskID);
            vector<ID> poses(set.begin(), set.end());
            auto noAP = atomic_proposition("TRUE");
            for (ID i = poses.size() - 1; i != -1; i--) {
                if (poses.at(i) >= actID) continue;
                auto& var = *activityVarMap.at(pair(taskID, poses.at(i)));
                auto p = (var == newValue);
                for (ID j = poses.at(i) + 1; j <= actID; j++)
                    mkDELA(taskID, j, ap & orderAP & noAP & p);
                noAP = noAP & (var != newValue);
            }
            if (!finish && actID < h - 1)
                mkADDA(taskID, actID + 1, newValue, ap & orderAP & noAP);
            if (finish) 
                mkADDA(taskID, actID, newValue, ap & orderAP & noAP);
        }
    }

    void NuXmv_ActivityAnalyzer::mkCTSK(Activity* activity, Intent* intent, 
                                        bool finish, ID taskID, ID actID,
                                        const atomic_proposition& ap) {
        auto orderAP = mkIsTopTaskAP(taskID);
        auto newActivity = intent -> getActivity();
        auto& newValue = *activityValueMap.at(newActivity);
        for (ID j = 1; j <= actID; j++)
            mkDELA(taskID, j, ap & orderAP);
        mkADDA(taskID, 0, newValue, ap & orderAP);
    }

    void NuXmv_ActivityAnalyzer::mkSIST(Activity* activity, Intent* intent, 
                                        bool finish, ID taskID, ID actID,
                                        const atomic_proposition& ap) {
        auto orderAP = mkIsTopTaskAP(taskID);
        auto newActivity = intent -> getActivity();
        auto& newValue = *activityValueMap.at(newActivity);
        auto newTaskID = getTaskID(newActivity);
        vector<Order> topOrders;
        getTopOrders(taskID, topOrders);
        bool f = actID == 0 && finish;
        for (auto& order : topOrders) {
            auto& value = *orderValueMap.at(order);
            auto orderAP = (orderVar == value);
            auto& realVar = *realActivityVarMap.at(newTaskID);
            auto& orderValue = getSwtOrderValue(order, newTaskID, f);
            add_transition(foa, orderVar, orderValue, ap & orderAP);
            if (order.at(newTaskID) == -1) {
                mkADDA(newTaskID, 0, newValue, ap & orderAP);
            }
            add_transition(foa, realVar, newValue, ap & orderAP);
            if (finish) mkDELA(taskID, actID, ap & orderAP);
        }
    }

    void NuXmv_ActivityAnalyzer::mkSWTK(Activity* activity, Intent* intent, 
                                        bool finish, ID taskID, ID actID,
                                        const atomic_proposition& ap,
                                        TaskPropMap& taskPropMap, bool real) {
        auto newActivity = intent -> getActivity();
        auto newTaskID = getTaskID(newActivity);
        auto& newValue = *activityValueMap.at(newActivity);
        vector<Order> topOrders;
        getTopOrders(taskID, topOrders);
        bool f = actID == 0 && finish;
        for (auto& order : topOrders) {
            auto& value = *orderValueMap.at(order);
            auto orderAP = (orderVar == value);
            auto& realVar = *realActivityVarMap.at(newTaskID);
            auto& orderValue = getSwtOrderValue(order, newTaskID, f);
            if (order.at(newTaskID) == -1) {
                add_transition(foa, orderVar, orderValue, ap & orderAP);
                add_transition(foa, realVar, newValue, ap & orderAP);
                mkADDA(newTaskID, 0, newValue, ap & orderAP);
            } else if (order.at(newTaskID) > 0) {
                auto realAP = atomic_proposition("TRUE");
                if (newActivity == mainActivity) realAP = (realVar != newValue | mainTaskVar == bool_value(1));
                else realAP = (realVar != newValue);
                // auto p = ap & orderAP & realAP;
                if (real) taskPropMap[order] = (ap & orderAP & realAP);
                else taskPropMap[order] = (ap & orderAP);
                add_transition(foa, orderVar, orderValue, ap & orderAP);
            }
        }
    }

    void NuXmv_ActivityAnalyzer::mkPUSH_N(Activity* activity, Intent* intent, 
                                          bool finish, ID taskID, ID actID,
                                          const atomic_proposition& ap) {
        auto newActivity = intent -> getActivity();
        auto& newValue = *(activityValueMap.at(newActivity));
        auto newTaskID = getTaskID(newActivity);
        if (taskID == newTaskID) {
            auto& realVar = *(realActivityVarMap.at(taskID));
            auto orderAP = mkIsTopTaskAP(taskID);
            auto realAP = atomic_proposition("TRUE");
            if (newActivity == mainActivity) realAP = realVar == newValue & mainTaskVar == bool_value(0);
            else realAP = realVar == newValue;
            if (finish) {
                mkADDA(taskID, actID, newValue, ap & orderAP & !realAP);
                mkDELA(taskID, actID, ap & orderAP & realAP);
                if (actID == 0) mkPOPT(taskID, ap & orderAP & realAP);
            } else {
                if (actID < h - 1)
                    mkADDA(taskID, actID + 1, newValue, ap & orderAP & !realAP);
                mkADDA(taskID, actID, newValue, ap & orderAP & realAP);
            }
            return;
        }
        TaskPropMap taskPropMap;
        mkSWTK(activity, intent, finish, taskID, actID, ap, taskPropMap);
        for (auto& [order, prop] : taskPropMap) {
            if (finish) mkDELA(taskID, actID, prop);
            if (legalPos.count(newActivity) > 0 &&
                legalPos.at(newActivity).count(newTaskID) > 0) {
                auto& js = legalPos.at(newActivity).at(newTaskID);
                for (auto j : js) {
                    if (j == 0) continue;
                    auto newTopAP = mkIsTopNullAP(newTaskID, j);
                    mkADDA(newTaskID, j, newValue, prop & newTopAP);
                }
            }
        }
    }

    void NuXmv_ActivityAnalyzer::mkSTOP_N(Activity* activity, Intent* intent, 
                                          bool finish, ID taskID, ID actID,
                                          const atomic_proposition& ap) {
        auto newActivity = intent -> getActivity();
        auto& newValue = *activityValueMap.at(newActivity);
        auto newTaskID = getTaskID(newActivity);
        if (taskID == newTaskID) {
            auto& realVar = *(realActivityVarMap.at(taskID));
            auto orderAP = mkIsTopTaskAP(taskID);
            auto realAP = atomic_proposition("TRUE");
            if (newActivity == mainActivity) realAP = realVar == newValue & mainTaskVar == bool_value(0);
            else realAP = realVar == newValue;
            if (activity == newActivity) {
                if (finish) {
                    mkDELA(taskID, actID, ap & orderAP);
                    if (actID == 0) mkPOPT(taskID, ap & orderAP);
                }
            } else {
                if (finish) {
                    mkADDA(taskID, actID, newValue, ap & orderAP & !realAP);
                    mkDELA(taskID, actID, ap & orderAP & realAP);
                    if (actID == 0) mkPOPT(taskID, ap & orderAP & realAP);
                } else {
                    if (actID < h - 1)
                        mkADDA(taskID, actID + 1, newValue, ap & orderAP & !realAP);
                    mkADDA(taskID, actID, newValue, ap & orderAP & realAP);
                }
            }
            return;
        }
        TaskPropMap taskPropMap;
        mkSWTK(activity, intent, finish, taskID, actID, ap, taskPropMap);
        for (auto& [order, prop] : taskPropMap) {
            if (finish) mkDELA(taskID, actID, prop);
            if (legalPos.count(newActivity) > 0 &&
                legalPos.at(newActivity).count(newTaskID) > 0) {
                auto& poses = legalPos.at(newActivity).at(newTaskID);
                for (auto j : poses) {
                    if (j == 0) continue;
                    auto& var = *activityVarMap.at(pair(newTaskID, j - 1));
                    auto newTopAP = mkIsTopNullAP(newTaskID, j);
                    mkADDA(newTaskID, j, newValue, prop & newTopAP & var != newValue);
                }
            }
        }
    }

    void NuXmv_ActivityAnalyzer::mkRTOF_N(Activity* activity, Intent* intent, 
                                          bool finish, ID taskID, ID actID,
                                          const atomic_proposition& ap) {
        auto newActivity = intent -> getActivity();
        auto& newValue = *activityValueMap.at(newActivity);
        auto newTaskID = getTaskID(newActivity);
        if (taskID == newTaskID) {
            mkRTOF(activity, intent, finish, taskID, actID, ap);
            return;
        }
        TaskPropMap taskPropMap;
        mkSWTK(activity, intent, finish, taskID, actID, ap, taskPropMap, 0);
        for (auto& [order, prop] : taskPropMap) {
            if (finish) mkDELA(taskID, actID, prop);
            if (legalPos.count(newActivity) == 0 ||
                legalPos.at(newActivity).count(newTaskID) == 0) return;
            auto& set = legalPos.at(newActivity).at(newTaskID);
            vector<ID> poses(set.begin(), set.end());
            for (ID actID = 1; actID < h; actID++) {
                auto topAP = mkIsTopNonNullAP(newTaskID, actID);
                auto noAP = atomic_proposition("TRUE");
                for (ID i = poses.size() - 1; i != -1; i--) {
                    if (poses.at(i) >= actID) continue;
                    auto& var = *activityVarMap.at(pair(newTaskID, poses.at(i)));
                    auto p = (var == newValue);
                    mkADDA(newTaskID, actID, newValue, prop & topAP & noAP & p);
                    for (ID j = poses.at(i); j < actID; j++) {
                        auto& var = *activityVarMap.at(pair(newTaskID, j + 1));
                        mkADDA(newTaskID, j, var, prop & topAP & noAP & p);
                    }
                    noAP = noAP & (var != newValue);
                }
                mkADDA(newTaskID, actID, newValue, prop & noAP & topAP);
            }
        }
    }

    void NuXmv_ActivityAnalyzer::mkCTOP_N(Activity* activity, Intent* intent, 
                                          bool finish, ID taskID, ID actID,
                                          const atomic_proposition& ap) {
        auto newActivity = intent -> getActivity();
        auto& newValue = *activityValueMap.at(newActivity);
        auto newTaskID = getTaskID(newActivity);
        if (taskID == newTaskID) {
            mkCTOP(activity, intent, finish, taskID, actID, ap);
            return;
        }
        TaskPropMap taskPropMap;
        mkSWTK(activity, intent, finish, taskID, actID, ap, taskPropMap, 0);
        for (auto& [order, prop] : taskPropMap) {
            if (finish) mkDELA(taskID, actID, prop);
            if (legalPos.count(newActivity) == 0 ||
                legalPos.at(newActivity).count(newTaskID) == 0) return;
            auto& set = legalPos.at(newActivity).at(newTaskID);
            vector<ID> poses(set.begin(), set.end());
            for (ID actID = 1; actID < h; actID++) {
                auto topAP = mkIsTopNullAP(newTaskID, actID);
                auto noAP = atomic_proposition("TRUE");
                for (ID i = poses.size() - 1; i != -1; i--) {
                    if (poses.at(i) >= actID) continue;
                    auto& var = *activityVarMap.at(pair(newTaskID, poses.at(i)));
                    auto p = (var == newValue);
                    for (ID j = poses.at(i) + 1; j <= actID; j++)
                        mkDELA(newTaskID, j, prop & topAP & noAP & p);
                    noAP = noAP & (var != newValue);
                }
                mkADDA(newTaskID, actID, newValue, prop & topAP & noAP);
            }
        }
    }

    void NuXmv_ActivityAnalyzer::mkCTSK_N(Activity* activity, Intent* intent, 
                                          bool finish, ID taskID, ID actID,
                                          const atomic_proposition& ap) {
        auto newActivity = intent -> getActivity();
        auto& newValue = *activityValueMap.at(newActivity);
        auto newTaskID = getTaskID(newActivity);
        if (taskID == newTaskID) {
            mkCTSK(activity, intent, finish, taskID, actID, ap);
            return;
        }
        TaskPropMap taskPropMap;
        mkSWTK(activity, intent, finish, taskID, actID, ap, taskPropMap, 0);
        for (auto& [order, prop] : taskPropMap) {
            for (ID j = 1; j < h; j++)
                mkDELA(newTaskID, j, prop);
            mkADDA(newTaskID, 0, newValue, prop);
        }
    }

    void NuXmv_ActivityAnalyzer::mkPOP() {
        for (ID i = 0; i < taskNum; i++) {
            auto orderAP = mkIsTopTaskAP(i);
            auto ap = actionVar == popValue & orderAP;
            if (taskNum == 1) orderAP = atomic_proposition("TRUE");
            auto hh = h;
            if (sitIDSet.count(i)) hh = 1;
            for (ID j = 0; j < hh; j++) {
                if (activityVarMap.count(pair(i,j)) == 0) break;
                auto& var = *activityVarMap.at(pair(i,j));
                auto actAP = mkIsTopNonNullAP(i,j);
                mkDELA(i, j, ap & actAP);
                if (j == 0) mkPOPT(i, (actionVar == popValue) & actAP);
                if (i == mainTaskID && j == 0) 
                    add_transition(foa, mainTaskVar, bool_value(0), ap & actAP);
            }
        }
    }

    void NuXmv_ActivityAnalyzer::translate2FOA() {
        mkPOP();
        for (auto& [activity, actions] : a -> getActionMap()) {
            for (auto& [intent, finish] : actions) {
                auto& actionValue = *actionValueMap.at(pair(activity, intent));
                auto actionAP = (actionVar == actionValue);
                if (legalPos.count(activity) == 0) continue;
                for (auto& [i, js] : legalPos.at(activity)) {
                    for (auto j : js) {
                        auto ap = actionAP & mkIsTopActAP(i, j, activity);
                        switch (AndroidStackMachine::getMode(activity, intent)) {
                            case SIST :
                                mkSIST(activity, intent, finish, i, j, ap);
                                break;
                            case PUSH :
                                mkPUSH(activity, intent, finish, i, j, ap);
                                break;
                            case STOP :
                                mkSTOP(activity, intent, finish, i, j, ap);
                                break;
                            case CTOP :
                                mkCTOP(activity, intent, finish, i, j, ap);
                                break;
                            case CTSK :
                                mkCTSK(activity, intent, finish, i, j, ap);
                                break;
                            case RTOF :
                                mkRTOF(activity, intent, finish, i, j, ap);
                                break;
                            case PUSH_N :
                                mkPUSH_N(activity, intent, finish, i, j, ap);
                                break;
                            case STOP_N :
                                mkSTOP_N(activity, intent, finish, i, j, ap);
                                break;
                            case CTOP_N :
                                mkCTOP_N(activity, intent, finish, i, j, ap);
                                break;
                            case CTSK_N :
                                mkCTSK_N(activity, intent, finish, i, j, ap);
                                break;
                            case RTOF_N :
                                mkRTOF_N(activity, intent, finish, i, j, ap);
                                break;
                            default :
                                break;
                        }
                    }
                }
            }
        }
        for (auto& [p, var] : activityVarMap)
            add_transition(foa, *var, *var, atomic_proposition("TRUE"));
        add_transition(foa, orderVar, orderVar, atomic_proposition("TRUE"));
        for (auto& [p, var] : realActivityVarMap)
            add_transition(foa, *var, *var, atomic_proposition("TRUE"));
        add_transition(foa, mainTaskVar, mainTaskVar, atomic_proposition("TRUE"));
    }

    bool NuXmv_ActivityAnalyzer::analyzeReachability(const atomic_proposition& ap,
                                                     std::ostream& os) {
        std::ifstream f("nuxmv_result");
        if (f) system("rm nuxmv_result");
        verify_invar_nuxmv(foa, ap, "nuxmv/source", 50, cmd);
        std::ifstream fin("nuxmv_result");
        if (!fin) return false;
        unordered_map<string, vector<string> > trace_table;
        parse_trace_nuxmv(foa, "nuxmv_result", trace_table);
        os << "-Activity Trace Found:" << endl;
        unordered_set<string> values;
        for (auto& value : trace_table.at("t")) {
            if (value == "pop") {
                os << "back" << endl;
            } else {
                os << value2ActionMap[value].first -> getName() << " -> " <<
                      value2ActionMap[value].second.first -> toString();
                if (value2ActionMap[value].second.second) os << " [finish]";
                os << endl;
                values.insert(value);
            }
        }
        os << "---Trace END---" << endl;
        fin.close();
        system("rm nuxmv_result && rm out.smv");
        return true;
    }
}

