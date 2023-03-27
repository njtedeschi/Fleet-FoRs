#pragma once

#include "Grammar.h"
#include "Singleton.h"

#include "DSL.h"

double TERMINAL_WEIGHT = 3.0;
double TERMINATING_WEIGHT = 1.0;
double UPWARD_WEIGHT = 1.0;
double RIGHTWARD_WEIGHT = 1.0;
double NEGATION_WEIGHT = 1.0;

class MyGrammar : public Grammar<MyInput,bool,   MyInput,bool,Object,Scene,Vector, Position, Direction, Displacement, double>,
				  public Singleton<MyGrammar> {
public:
	MyGrammar() {
                add("displacement(%s,%s)", DSL::displacement);

		add("forward(%s)", DSL::forward);
		/* add("upward(%s)", DSL::upward, UPWARD_WEIGHT*VECTOR_WEIGHT); */
		add("rightward(%s)", DSL::rightward, RIGHTWARD_WEIGHT);

                add("parallel(%s,%s)", DSL::parallel, TERMINATING_WEIGHT);
                add("+-parallel(%s,%s)", +[](Displacement x, Direction y) -> bool {
                    if (DSL::nonzero(x,y)) {return false;}
                    return cosine_similarity(x,y) == 1 || cosine_similarity(x,y) == -1;
                        }, TERMINATING_WEIGHT);
                /* add("antiparallel(%s,%s)", DSL::antiparallel); */
                /* add("orthogonal(%s,%s)", DSL::orthogonal); */

                // Functions only used for data generation
                add("Parallel(%s,%s)", DSL::parallel_orientation, 0.0);
                add("Antiparallel(%s,%s)", DSL::antiparallel_orientation, 0.0);
		
                add("and(%s,%s)",    Builtins::And<MyGrammar>);
		add("or(%s,%s)",     Builtins::Or<MyGrammar>);
		add("not(%s)",       Builtins::Not<MyGrammar>, NEGATION_WEIGHT);


                add("scene(%s)", +[](MyInput x) -> Scene {return x.scene;}, TERMINAL_WEIGHT);
		add("UP(%s)",       +[](Scene x) -> Direction {return Space::up;}, UPWARD_WEIGHT);
		add("S(%s)",       +[](Scene x) -> Object { return x.speaker; });


		add("F(%s)",        +[](MyInput x) -> Object { return x.scene.figure; }, TERMINAL_WEIGHT);
		add("G(%s)",        +[](MyInput x) -> Object { return x.scene.ground; }, TERMINAL_WEIGHT);
                add("G'(%s)", +[](MyInput x) -> Object {
                        if(x.scene.ground.is_participant) {
                            return Space::invalid_object;
                        }
                        return x.scene.ground;
                        }, TERMINAL_WEIGHT);
		add("x",             Builtins::X<MyGrammar>);
	}
} grammar;
