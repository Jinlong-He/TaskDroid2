//
//  LoopAnalyzer.hpp
//  TaskDroid 
//
//  Licensed under the MIT License <http://opensource.org/licenses/MIT>.
//  SPDX-License-Identifier: MIT
//  Copyright (c) 2021 Jinlong He.
//

#ifndef LoopAnalyzer_hpp 
#define LoopAnalyzer_hpp 

#include <queue>
#include <iostream>
#include "../AndroidStackMachine/AndroidStackMachine.hpp"
using std::queue;
namespace TaskDroid {
    template <class Symbol, class Weight>
    class LoopAnalyzer {
    public:
        typedef vector<pair<Symbol, Weight> > Path;
        typedef vector<Path> Paths;
        typedef unordered_map<Symbol, Paths> PathsMap;
        typedef unordered_map<Symbol, unordered_map<Symbol, Weight> > Graph;

        static void getPositiveLoop(const Graph& graph, Paths& loops) {
            Paths newLoops;
            getLoop(graph, newLoops);
            for (auto& loop : newLoops) {
                //int count = int(graph.at(loop[loop.size() - 1].first).at(loop[0].first));
                int count = 0;
                for (ID i = 0; i < loop.size() - 1; i++) 
                    count += int(graph.at(loop[i].first).at(loop[i + 1].first));
                if (count > 0) loops.push_back(loop);
            }
        }

        static void getLoop(const Graph& graph, Paths& loops) {
            for (auto& [symbol, map] : graph) {
                Paths newLoops, newerLoops;
                getLoop(graph, symbol, newLoops);
                if (loops.size() == 0) loops = newLoops;
                for (auto& newLoop : newLoops) {
                    bool flag = true;
                    for (auto& loop : loops) 
                        if (isSame(loop, newLoop))
                            flag = false;
                    if (flag) newerLoops.push_back(newLoop);
                }
                loops.insert(loops.end(), newerLoops.begin(), newerLoops.end());
            }
        }

        static bool isSame(const Path& path1_, const Path& path2_) {
            if (path1_.size() != path2_.size()) return false;
            Path path1(path1_.begin() + 1, path1_.end());
            Path path2(path2_.begin() + 1, path2_.end());
            for (ID i = 0; i < path1.size(); i++) {
                if (path1[i] == path2[0]) {
                    bool flag = true;
                    for (ID j = i; j < path1.size(); j++) {
                        if (path1[j] != path2[j - i]) {
                            flag = false;
                            break;
                        }
                    }
                    if (!flag) continue;
                    for (ID j = 0; j < i; j++) {
                        if (path1[j] != path2[j + path1.size() - i]) {
                            flag = false;
                            break;
                        }
                    }
                    if (flag) return true;
                }
            }
            return false;
        }

        static void getLoop(const Graph& graph, Symbol init, Paths& loops) {
            queue<Symbol> q({init});
            unordered_set<Symbol> visited({init}), sources;
            PathsMap pathsMap({{init, Paths({Path({pair(init, Weight())})})}});
            while (!q.empty()) {
                auto source = q.front();
                q.pop();
                if (graph.count(source) == 0) continue;
                auto& paths = pathsMap.at(source);
                for (auto& [target, weight] : graph.at(source)) {
                    if (target == init) {
                        sources.insert(source);
                    }
                    if (!visited.insert(target).second) continue;
                    q.push(target);
                    auto& newPaths = pathsMap[target];
                    for (auto& path : paths) {
                        Path newPath = path;
                        newPath.push_back(pair(target, weight));
                        newPaths.push_back(newPath);
                    }
                }
            }
            for (auto source : sources) {
                auto& paths = pathsMap.at(source);
                for (auto& path : paths) {
                    auto act = path[path.size() - 1].first;
                    path.emplace_back(pair(init, graph.at(act).at(init)));
                }
                loops.insert(loops.end(), paths.begin(), paths.end());
            }
        }

    private:
    };
}
#endif /* LoopAnalyzer_hpp */
