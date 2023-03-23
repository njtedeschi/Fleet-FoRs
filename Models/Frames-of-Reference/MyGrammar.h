#pragma once

#include "Grammar.h"
#include "Singleton.h"

#include "DSL.h"

double VECTOR_WEIGHT = 3.0;
double UPWARD_WEIGHT = 1.0;
double RIGHTWARD_WEIGHT = 1.0;

class MyGrammar : public Grammar<MyInput,bool,   MyInput,bool,Object,Vector, Position, Direction, Displacement, double>,
				  public Singleton<MyGrammar> {
public:
	MyGrammar() {
                add("displacement(%s,%s)", DSL::displacement, VECTOR_WEIGHT);

		add("forward(%s)", DSL::forward, VECTOR_WEIGHT);
		add("upward(%s)", DSL::upward, UPWARD_WEIGHT*VECTOR_WEIGHT);
		add("rightward(%s)", DSL::rightward, RIGHTWARD_WEIGHT*VECTOR_WEIGHT);

                add("parallel(%s,%s)", DSL::parallel);
                /* add("antiparallel(%s,%s)", DSL::antiparallel); */
                /* add("orthogonal(%s,%s)", DSL::orthogonal); */

                // Functions only used for data generation
                add("Parallel(%s,%s)", DSL::parallel_orientation, 0.0);
                add("Antiparallel(%s,%s)", DSL::antiparallel_orientation, 0.0);
		
                add("and(%s,%s)",    Builtins::And<MyGrammar>);
		add("or(%s,%s)",     Builtins::Or<MyGrammar>);
		add("not(%s)",       Builtins::Not<MyGrammar>);

		add("UP(%s)",       +[](MyInput x) -> Direction {return Space::up;}, UPWARD_WEIGHT*VECTOR_WEIGHT);

		add("S(%s)",       +[](MyInput x) -> Object { return x.scene.speaker; });
		add("F(%s)",        +[](MyInput x) -> Object { return x.scene.figure; });
		add("G(%s)",        +[](MyInput x) -> Object { return x.scene.ground; });
                add("G'(%s)", +[](MyInput x) -> Object {
                        if(!x.scene.is_direct) {
                            return x.scene.ground;
                        }
                        else {
                            return Space::invalid_object;
                        }
                        });
		add("x",             Builtins::X<MyGrammar>);
	}
} grammar;
