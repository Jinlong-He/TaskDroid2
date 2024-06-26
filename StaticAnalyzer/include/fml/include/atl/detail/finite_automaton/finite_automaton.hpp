//
//  finite_automaton.hpp
//  ATL 
//
//  Licensed under the MIT License <http://opensource.org/licenses/MIT>.
//  SPDX-License-Identifier: MIT
//  Copyright (c) 2020 Jinlong He.
//

#ifndef atl_detail_finite_automaton_hpp 
#define atl_detail_finite_automaton_hpp

#include <unordered_set>
#include <unordered_map>
#include <util/util.hpp>
#include <atl/detail/automaton.hpp>
#include <atl/detail/no_type.hpp>

using std::unordered_map, std::unordered_set;

namespace atl::detail {
    template <class Symbol, 
              class SymbolProperty,
              class StateProperty, 
              class AutomatonProperty> class nondeterministic_finite_automaton_gen;

    template <class Symbol, 
              class SymbolProperty,
              class StateProperty, 
              class AutomatonProperty> class deterministic_finite_automaton_gen;


    template <class Symbol, 
              class SymbolProperty,
              class StateProperty, 
              class AutomatonProperty>
    class finite_automaton_gen 
        : public automaton_gen<
                 typename std::conditional<std::is_same<SymbolProperty, no_type>::value,
                               Symbol, Property<Symbol, SymbolProperty> >::type,
                 typename std::conditional<std::is_same<StateProperty, no_type>::value,
                               boost::no_property, StateProperty>::type,
                 typename std::conditional<std::is_same<AutomatonProperty, no_type>::value,
                               boost::no_property, AutomatonProperty>::type> {
    public:
        typedef Property<Symbol, SymbolProperty> transition_property;
        typedef Symbol symbol_type;
        typedef SymbolProperty symbol_property_type;

        typedef typename std::conditional<std::is_same<SymbolProperty, no_type>::value,
                              Symbol, transition_property>::type 
            transition_property_type;
        typedef typename std::conditional<std::is_same<StateProperty, no_type>::value,
                               boost::no_property, StateProperty>::type
            state_property_type;
        typedef typename std::conditional<std::is_same<AutomatonProperty, no_type>::value,
                               boost::no_property, AutomatonProperty>::type
            automaton_property_type;
        typedef automaton_gen<transition_property_type, 
                              state_property_type, 
                              automaton_property_type> Base;

        typedef deterministic_finite_automaton_gen<Symbol, 
                                                   SymbolProperty,
                                                   StateProperty,
                                                   AutomatonProperty> dfa_type;

        typedef nondeterministic_finite_automaton_gen<Symbol, 
                                                      SymbolProperty,
                                                      StateProperty,
                                                      AutomatonProperty> nfa_type;

        typedef typename Base::State State;
        typedef typename Base::Transition Transition;
        typedef typename Base::InTransitionIter InTransitionIter;
        typedef typename Base::OutTransitionIter OutTransitionIter;

        typedef unordered_set<State> StateSet;
        typedef unordered_set<Symbol> SymbolSet;
        typedef unordered_map<State, State> State2Map;

    public:
        finite_automaton_gen()
            : Base(),
              initial_state_(-1),
              alphabet_(SymbolSet()),
              epsilon_(Symbol()) {}

        finite_automaton_gen(const SymbolSet alphabet,
                             const Symbol& epsilon)
            : Base(),
              initial_state_(-1),
              alphabet_(alphabet),
              epsilon_(epsilon) {}

        finite_automaton_gen(const std::initializer_list<Symbol> alphabet,
                             const Symbol& epsilon)
            : Base(),
              initial_state_(-1),
              alphabet_(alphabet),
              epsilon_(epsilon) {}

        finite_automaton_gen(const finite_automaton_gen& x)
            : Base(x),
              initial_state_(x.initial_state_),
              final_state_set_(x.final_state_set_),
              state_set_(x.state_set_),
              alphabet_(x.alphabet_),
              epsilon_(x.epsilon_) {}

        ~finite_automaton_gen() {}

        finite_automaton_gen& 
        operator=(const finite_automaton_gen& x) {
            if (&x != this) {
                Base::operator=(x);
                initial_state_ = x.initial_state_;
                final_state_set_ = x.final_state_set_;
                state_set_ = x.state_set_;
                alphabet_ = x.alphabet_;
            }
            return *this;
        }

        virtual void clear() {
            Base::clear();
            initial_state_ = -1;
            final_state_set_.clear();
            state_set_.clear();
            alphabet_.clear();
        }

        virtual State 
        add_state(const state_property_type& p) {
            State state = Base::add_state(p);
            state_set_.insert(state);
            return state;
        }

        virtual State 
        add_state() {
            State state = Base::add_state();
            state_set_.insert(state);
            return state;
        }

        virtual void 
        clear_state(State s) {
            clear_out_transitions(s);
            clear_in_transitions(s);
            Base::clear_state(s);
        }
                        
        virtual void 
        clear_out_transitions(State s) {
            Base::clear_out_transitions(s);
        }
                        
        virtual void 
        clear_in_transitions(State s) {
            InTransitionIter first, last;
            StateSet del_states;
            tie(first, last) = this -> in_transitions(s);
            for (; first != last; first++) {
                del_states.insert(this -> source(*first));
            }
            for (auto del_state : del_states) {
                remove_transition(del_state, s);
            }
        }

        virtual void 
        remove_state(State s) {
            state_set_.erase(s);
            Base::remove_state(s);
        }

        const Symbol&
        epsilon() const {
            return epsilon_;
        }

        void set_epsilon(const Symbol& epsilon) {
            epsilon_ = epsilon;
        }

        transition_property_type
        epsilon_transition() const {
            return transition_property_type(epsilon_);
        }

        State
        initial_state() const {
            return initial_state_;
        }

        void 
        set_initial_state(State state) {
            initial_state_ = state;
        }

        const StateSet&
        state_set() const {
            return state_set_;
        }

        const StateSet&
        final_state_set() const {
            return final_state_set_;
        }

        void
        set_final_state_set(const StateSet& final_state_set) {
            final_state_set_ = final_state_set;
        }

        void
        clear_final_state_set() {
            final_state_set_.clear();
        }

        const SymbolSet&
        alphabet() const {
            return alphabet_;
        }

        virtual void
        set_alphabet(const SymbolSet& alphabet) {
            alphabet_ = alphabet;
        }

        virtual void 
        add_alphabet(const Symbol& c) {
            alphabet_.insert(c);
        }

        void 
        set_final_state(State state) {
            final_state_set_.insert(state);
        }

        void 
        remove_final_state(State state) {
            final_state_set_.erase(state);
        }
        
        using Base::add_transition;
        virtual pair<Transition, bool>
        add_transition(State s, State t,
                       const transition_property_type& c) {
            if constexpr (std::is_same<SymbolProperty, no_type>::value) {
                if (c == epsilon()) this -> set_flag(4, 1);
            } else {
                if (c.default_property == epsilon()) this -> set_flag(4, 1);
            }
            return Base::add_transition(s, t, c);
        }

        virtual pair<Transition, bool>
        add_transition(State s, State t,
                       const Symbol& c,
                       const SymbolProperty& p) {
            if constexpr (std::is_same<SymbolProperty, no_type>::value) {
                if (c == epsilon()) this -> set_flag(4, 1);
                return Base::add_transition(s, t, c);
            } else {
                if (c == epsilon()) this -> set_flag(4, 1);
                return Base::add_transition(s, t, transition_property(c, p));
            }
        }

        virtual void
        remove_transition(Transition t) {
            Base::remove_transition(t);
        }

        virtual void
        remove_transition(State s, State t) {
            Base::remove_transition(s, t);
        }

    private:
        State initial_state_;
        StateSet final_state_set_;
        StateSet state_set_;
        SymbolSet alphabet_;
        Symbol epsilon_;
    };
};

namespace atl {
    #define FA_PARAMS typename FA_SYMBOL, typename FA_SYMBOL_PROP, typename FA_STATE_PROP, typename FA_AUT_PROP
    #define FA detail::finite_automaton_gen<FA_SYMBOL, FA_SYMBOL_PROP, FA_STATE_PROP,FA_AUT_PROP>

    template <FA_PARAMS>
    inline typename FA::SymbolSet const&
    alphabet(const FA& fa) {
        return fa.alphabet();
    }

    template <FA_PARAMS>
    inline void
    set_alphabet(FA& fa,
                 typename FA::SymbolSet const& set) {
        fa.set_alphabet(set);
    }

    template <FA_PARAMS>
    inline typename FA::State
    initial_state(const FA& fa) {
        return fa.initial_state();
    }

    template <FA_PARAMS>
    inline void
    set_initial_state(FA& fa, 
                      typename FA::State state) {
        fa.set_initial_state(state);
    }

    template <FA_PARAMS>
    inline typename FA::StateSet const&
    state_set(const FA& fa) {
        return fa.state_set();
    }

    template <FA_PARAMS>
    inline typename FA::StateSet const&
    final_state_set(const FA& fa) {
        return fa.final_state_set();
    }

    template <FA_PARAMS>
    inline void  
    set_final_state_set(FA& fa, 
                        typename FA::StateSet const& set) {
        fa.set_final_state_set(set);
    }

    template <FA_PARAMS>
    inline void  
    set_final_state(FA& fa, 
                    typename FA::State state) {
        fa.set_final_state(state);
    }
    
    template <FA_PARAMS>
    inline void  
    remove_final_state(FA& fa, 
                       typename FA::State state) {
        fa.remove_final_state(state);
    }

    template <FA_PARAMS>
    inline void  
    clear_final_state_set(FA& fa) {
        fa.clear_final_state_set();
    }

    template <FA_PARAMS>
    inline FA_SYMBOL const&
    epsilon(const FA& fa) {
        return fa.epsilon();
    }

    template <FA_PARAMS>
    inline void
    set_epsilon(const FA& fa,
                const FA_SYMBOL& epsilon) {
        return fa.set_epsilon(epsilon);
    }

    template <FA_PARAMS>
    inline typename FA::transition_property_type
    epsilon_transition(const FA& fa) {
        return fa.epsilon_transition();
    }

    template <FA_PARAMS>
    inline void
    add_alphabet(FA& fa,
                 const FA_SYMBOL& c) {
        fa.add_alphabet(c);
    }

    template <FA_PARAMS>
    inline typename FA::State
    add_initial_state(FA& fa,
                      typename FA::state_property_type const& p) {
        typename FA::State s = add_state(fa, p);
        fa.set_initial_state(s);
        return s;
    }

    template <FA_PARAMS>
    inline typename FA::State
    add_initial_state(FA& fa) {
        typename FA::State s = add_state(fa);
        fa.set_initial_state(s);
        return s;
    }

    template <FA_PARAMS>
    inline typename FA::State
    add_final_state(FA& fa,
                    typename FA::state_property_type const& p) {
        typename FA::State s = add_state(fa, p);
        fa.set_final_state(s);
        return s;
    }

    template <FA_PARAMS>
    inline typename FA::State
    add_final_state(FA& fa) {
        typename FA::State s = add_state(fa);
        fa.set_final_state(s);
        return s;
    }

    template <FA_PARAMS>
    inline pair<typename FA::Transition, bool>
    add_transition(FA& fa,
                   typename FA::State s,
                   typename FA::State t,
                   const FA_SYMBOL& c,
                   typename FA::symbol_property_type const& p) {
        return fa.add_transition(s, t, c, p);
    }
    
    template <FA_PARAMS>
    inline bool
    is_initial_state(const FA& fa,
                     typename FA::State s) {
        return (s == initial_state(fa));
    }

    template <FA_PARAMS>
    inline bool
    is_final_state(const FA& fa,
                   typename FA::State s) {
        return fa.final_state_set().count(s);
    }

    template <FA_PARAMS>
    inline bool
    has_final_state(const FA& fa,
                   typename FA::StateSet const& set) {
        typename FA::StateSet res;
        util::set_intersection(fa.final_state_set(), set, res);
        return res.size();
    }

    template <FA_PARAMS>
    inline void 
    set_forward_reachable_flag(FA& a, bool b = true) {
        a.set_flag(1, b);
        a.set_flag(0, 0);
    }

    template <FA_PARAMS>
    inline bool
    is_forward_reachable(const FA& a) {
        return (a.flag(1) & !a.flag(0));
    }

    template <FA_PARAMS>
    inline void 
    set_minimal_flag(FA& a, bool b = true) {
        a.set_flag(2, b);
        a.set_flag(1, 1);
        a.set_flag(0, 0);
    }

    template <FA_PARAMS>
    inline bool
    is_minimal(const FA& a) {
        return (a.flag(2) & !a.flag(0));
    }

    template <FA_PARAMS>
    inline void 
    set_undeterministic_flag(FA& a, bool b = true) {
        a.set_flag(3, b);
    }

    template <FA_PARAMS>
    inline bool
    is_undeterministic(const FA& a) {
        return (a.flag(3) | a.flag(4));
    }

    template <FA_PARAMS>
    inline void 
    set_epsilon_flag(FA& a, bool b = true) {
        a.set_flag(4, b);
    }

    template <FA_PARAMS>
    inline bool
    has_epsilon_transition(const FA& a) {
        return a.flag(4);
    }

};

#endif /* atl_detail_finite_automaton_hpp */
