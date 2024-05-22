#include "AndroidStackMachine/Configuration.hpp"
#include "Activity.hpp"
#include "Fragment.hpp"
#include "FragmentTransaction.hpp"
#include <iostream>
using std::cout, std::endl;
namespace TaskDroid {
    const vector<Fragment*>& FragmentContainer::getContent() const {
        return content;
    }

    const string& FragmentContainer::getViewID() const {
        return viewID;
    }

    const vector<FragmentTransaction*>& FragmentBackStack::getContent() const {
        return content;
    }

    const vector<FragmentContainer>& ActivityConfiguration::getFragmentContainers() const {
        return fragmentContainers;
    }

    const FragmentBackStack& ActivityConfiguration::getFragmentBackStack() const {
        return fragmentBackStack;
    }

    Activity* ActivityConfiguration::getActivity() const {
        return activity;
    }

    const vector<ActivityConfiguration>& Task::getContent() const {
        return content;
    }

    Activity* Task::getRealActivity() const {
        return realActivity;
    }

    const vector<Task>& Configuration::getContent() const {
        return content;
    }
}
