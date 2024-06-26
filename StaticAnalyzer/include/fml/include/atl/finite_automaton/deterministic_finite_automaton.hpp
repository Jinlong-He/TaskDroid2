//
//  deterministic_finite_automaton.hpp
//  atl 
//
//  Licensed under the MIT License <http://opensource.org/licenses/MIT>.
//  SPDX-License-Identifier: MIT
//  Copyright (c) 2020 Jinlong He.
//

#ifndef atl_deterministic_finite_automaton_hpp 
#define atl_deterministic_finite_automaton_hpp

#include <atl/detail/finite_automaton/deterministic_finite_automaton.hpp>
#include <atl/detail/finite_automaton/algorithm.hpp>
#include <atl/detail/finite_automaton/operate.hpp>
#include <atl/detail/finite_automaton/cast.hpp>
#include <initializer_list>

namespace atl {
    template <class Symbol = char, 
              class SymbolProperty = no_type,
              class StateProperty = no_type, 
              class AutomatonProperty = no_type>
    class deterministic_finite_automaton
        : public detail::deterministic_finite_automaton_gen<Symbol,
                                                            SymbolProperty,
                                                            StateProperty,
                                                            AutomatonProperty> {
        public:
            typedef Symbol symbol_type;
            typedef detail::deterministic_finite_automaton_gen<Symbol,
                                                               SymbolProperty,
                                                               StateProperty,
                                                               AutomatonProperty> Base;

            typedef detail::finite_automaton_gen<Symbol,
                                                 SymbolProperty,
                                                 StateProperty,
                                                 AutomatonProperty> fa_type;
            typedef deterministic_finite_automaton<Symbol,
                                                   SymbolProperty,
                                                   StateProperty,
                                                   AutomatonProperty> dfa_type;
            typedef detail::nondeterministic_finite_automaton_gen<Symbol,
                                                                  SymbolProperty,
                                                                  StateProperty,
                                                                  AutomatonProperty> 
                        nfa_type;

            typedef typename Base::state_property_type state_property_type;
            typedef typename Base::automaton_property_type automaton_property_type;
            typedef typename Base::transition_property_type transition_property_type;

            typedef typename Base::State State;
            typedef typename Base::StateSet StateSet;
            typedef typename Base::StatePair StatePair;
            typedef typename Base::SymbolSet SymbolSet;
            typedef typename Base::Transition Transition;
            typedef typename Base::TransitionMap TransitionMap;
            typedef typename Base::Symbol2StateMap Symbol2StateMap;
            typedef typename Base::Symbol2StatePairMap Symbol2StatePairMap;

        public:
            deterministic_finite_automaton(const SymbolSet& alphabet = SymbolSet(), 
                                           const Symbol& epsilon = Symbol())
                : Base(alphabet, epsilon) {}

            deterministic_finite_automaton(const std::initializer_list<Symbol>& alphabet,
                                           const Symbol& epsilon = Symbol())
                : Base(alphabet, epsilon) {}

            deterministic_finite_automaton(const deterministic_finite_automaton& x)
                : Base(x) {}

            deterministic_finite_automaton(const Base& x)
                : Base(x) {}

            ~deterministic_finite_automaton() {}

            dfa_type& 
            operator=(const dfa_type& x) {
                if (&x != this) {
                    Base::operator=(x);
                }
                return *this;
            }

            dfa_type& 
            operator=(const Base& x) {
                if (&x != this) {
                    Base::operator=(x);
                }
                return *this;
            }

            dfa_type
            operator&(const dfa_type& x) {
                dfa_type out, dfa_lhs, dfa_rhs;
                minimize(*this, dfa_lhs);
                minimize(x, dfa_rhs);
                intersect_fa(dfa_lhs, dfa_rhs, out);
                return out;
            }

            dfa_type
            operator&(const nfa_type& x) {
                dfa_type out, dfa_lhs, dfa_rhs;
                minimize(*this, dfa_lhs);
                minimize(x, dfa_rhs);
                intersect_fa(dfa_lhs, dfa_rhs, out);
                return out;
            }
            
            dfa_type
            operator|(const dfa_type& x) {
                dfa_type out, dfa_lhs, dfa_rhs;
                minimize(*this, dfa_lhs);
                minimize(x, dfa_rhs);
                union_fa(dfa_lhs, dfa_rhs, out);
                return out;
            }

            dfa_type
            operator|(const nfa_type& x) {
                dfa_type out, dfa_lhs, dfa_rhs;
                minimize(*this, dfa_lhs);
                minimize(x, dfa_rhs);
                union_fa(dfa_lhs, dfa_rhs, out);
                return out;
            }

            dfa_type
            operator-(const dfa_type& x) {
                dfa_type out, dfa_lhs, dfa_rhs, dfa;
                minimize(*this, dfa_lhs);
                minimize(x, dfa_rhs);
                complement_fa(dfa_rhs, dfa);
                intersect_fa(dfa_lhs, dfa, out);
                return out;
            }

            dfa_type
            operator-(const nfa_type& x) {
                dfa_type out, dfa_lhs, dfa_rhs, dfa;
                minimize(*this, dfa_lhs);
                minimize(x, dfa_rhs);
                complement_fa(dfa_rhs, dfa);
                intersect_fa(dfa_lhs, dfa, out);
                return out;
            }
            
            dfa_type
            operator+(const dfa_type& x) {
                dfa_type out, dfa_lhs, dfa_rhs;
                minimize(*this, dfa_lhs);
                minimize(x, dfa_rhs);
                concat_fa(dfa_lhs, dfa_rhs, out);
                return out;
            }

            dfa_type
            operator+(const nfa_type& x) {
                dfa_type out, dfa_lhs, dfa_rhs;
                minimize(*this, dfa_lhs);
                minimize(x, dfa_rhs);
                concat_fa(dfa_lhs, dfa_rhs, out);
                return out;
            }

            dfa_type
            operator!() {
                dfa_type dfa, out;
                minimize(*this, dfa);
                complement_fa(dfa, out);
                return out;
            }

            bool
            operator==(const dfa_type& x) {
                dfa_type dfa_lhs, dfa_rhs;
                minimize(*this, dfa_lhs);
                minimize(x, dfa_rhs);
                return equal_fa(dfa_lhs, dfa_rhs);
            }

            bool
            operator==(const nfa_type& x) {
                dfa_type dfa_lhs, dfa_rhs;
                minimize(*this, dfa_lhs);
                minimize(x, dfa_rhs);
                return equal_fa(dfa_lhs, dfa_rhs);
            }

            bool
            operator<=(const dfa_type& x) {
                dfa_type dfa_lhs, dfa_rhs;
                minimize(*this, dfa_lhs);
                minimize(x, dfa_rhs);
                return is_empty(dfa_rhs - dfa_lhs);
            }

            bool
            operator<=(const nfa_type& x) {
                dfa_type dfa_lhs, dfa_rhs;
                minimize(*this, dfa_lhs);
                minimize(x, dfa_rhs);
                return is_empty(dfa_rhs - dfa_lhs);
            }
        private:
        };
}

#endif /* atl_deterministic_finite_automaton_hpp */
