//
//  Configuration.hpp
//  TaskDroid 
//
//  Licensed under the MIT License <http://opensource.org/licenses/MIT>.
//  SPDX-License-Identifier: MIT
//  Copyright (c) 2021 Jinlong He.
//

#ifndef Configuration_hpp 
#define Configuration_hpp 

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "AndroidStackMachine.hpp"
using std::string, std::vector, std::pair;
namespace TaskDroid {
    class FragmentContainer {
    public:
        FragmentContainer()
            : viewID("") {}

        FragmentContainer(const vector<Fragment*>& content_, const string& viewID_)
            : content(content_),
              viewID(viewID_) {}
        const vector<Fragment*>& getContent() const;
        const string& getViewID() const;
    private:
        vector<Fragment*> content;
        string viewID;
    };

    class FragmentBackStack {
    public:
        FragmentBackStack() {}
        const vector<FragmentTransaction*>& getContent() const;
    private:
        vector<FragmentTransaction*> content;
    };

    class ActivityConfiguration {
    public:
        ActivityConfiguration()
            : fragmentContainers(),
              fragmentBackStack(),
              activity(nullptr) {}

        ActivityConfiguration(Activity* activity_)
            : fragmentContainers(),
              fragmentBackStack(),
              activity(activity_) {}

        ActivityConfiguration(const vector<FragmentContainer>& fragmentContainers_,
                              const FragmentBackStack& fragmentBackStack_,
                              Activity* activity_)
            : fragmentContainers(fragmentContainers_),
              fragmentBackStack(fragmentBackStack_),
              activity(activity_) {}
        const vector<FragmentContainer>& getFragmentContainers() const;
        const FragmentBackStack& getFragmentBackStack() const;
        Activity* getActivity() const;
    private:
        vector<FragmentContainer> fragmentContainers;
        FragmentBackStack fragmentBackStack;
        Activity* activity;
    };

    class Task {
    public:
        Task()
            : realActivity(nullptr) {}

        Task(Activity* realActivity_,
             const vector<ActivityConfiguration>& content_)
            : realActivity(realActivity_),
              content(content_) {}

        Activity* getRealActivity() const;
        const vector<ActivityConfiguration>& getContent() const;
    private:
        Activity* realActivity;
        vector<ActivityConfiguration> content;
    };

    class Configuration {
    public:
        Configuration() {}

        Configuration(const vector<Task>& content_)
            : content(content_) {}

        Configuration(const AndroidStackMachine& a, const string& file) {
            std::ifstream fin(file);
            string line = "";
            while (getline(fin, line)) {
                if (line[0] == '\"') continue;
                auto taskStrs = util::split(line, ",");
                auto actStrs = util::split(taskStrs.at(1), " ");
                vector<ActivityConfiguration> acs;
                for (auto& actStr : actStrs)
                    acs.emplace_back(ActivityConfiguration(a.getActivity(actStr)));
                Task task(a.getActivity(taskStrs.at(0)), acs);
                content.emplace_back(task);
            }
            fin.close();
        }

        const vector<Task>& getContent() const;
    private:
        vector<Task> content;
    };
}
#endif /* Configuration_hpp */
