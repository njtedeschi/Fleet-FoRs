#pragma once

#include "Grammar.h"
#include "Singleton.h"

#include "DSL.h"

double VECTOR_WEIGHT = 3.0;

class MyGrammar : public Grammar<MyInput,bool,   MyInput,bool,Object,Vector, double>,
				  public Singleton<MyGrammar> {
public:
	MyGrammar() {
                add("displacement(%s,%s)", DSL::displacement, VECTOR_WEIGHT);

		add("forward(%s)", DSL::forward, VECTOR_WEIGHT);
		add("upward(%s)", DSL::upward, VECTOR_WEIGHT);
		add("rightward(%s)", DSL::rightward, VECTOR_WEIGHT);

                add("parallel(%s,%s)", DSL::parallel);
                /* add("antiparallel(%s,%s)", DSL::antiparallel); */
                add("orthogonal(%s,%s)", DSL::orthogonal);
		
                add("and(%s,%s)",    Builtins::And<MyGrammar>);
		add("or(%s,%s)",     Builtins::Or<MyGrammar>);
		add("not(%s)",       Builtins::Not<MyGrammar>);

		add("UP(%s)",       +[](MyInput x) -> Vector { Vector up = {0,0,1}; return up;}, VECTOR_WEIGHT);

		add("S(%s)",       +[](MyInput x) -> Object { return x.scene.speaker; });
		add("F(%s)",        +[](MyInput x) -> Object { return x.scene.figure; });
		add("G(%s)",        +[](MyInput x) -> Object { return x.scene.ground; });
		add("x",             Builtins::X<MyGrammar>);
	}
} grammar;
