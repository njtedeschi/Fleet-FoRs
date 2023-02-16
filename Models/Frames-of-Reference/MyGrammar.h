#pragma once

#include "Grammar.h"
#include "Singleton.h"

class MyGrammar : public Grammar<MyInput,bool,   MyInput,bool,Object,Vector, double>,
				  public Singleton<MyGrammar> {
private:
        Vector up = {0, 0, 1};
        Vector down = {0, 0, -1};
public:
	MyGrammar() {
                add("displacement(%s,%s)", +[](Object x, Object y) -> Vector {
                        return y.location - x.location;
                        });
		add("orientation(%s)", +[](Object x) -> Vector {return x.orientation;});
                add("parallel(%s,%s)", +[](Vector x, Vector y) -> bool {
                        if (magnitude(x) == 0 || magnitude(y) == 0) {return false;}
                        return cosine_similarity(x,y) == 1;
                        });
                /* add("antiparallel(%s,%s)", +[](Vector x, Vector y) -> bool { */
                /*         if (magnitude(x) == 0 || magnitude(y) == 0) {return false;} */
                /*         return cosine_similarity(x,y) == -1; */
                /*         }); */
                add("orthogonal(%s,%s)", +[](Vector x, Vector y) -> bool {
                        if (magnitude(x) == 0 || magnitude(y) == 0) {return false;}
                        return cosine_similarity(x,y) == 0;
                        });
		
                add("and(%s,%s)",    Builtins::And<MyGrammar>);
		add("or(%s,%s)",     Builtins::Or<MyGrammar>);
		add("not(%s)",       Builtins::Not<MyGrammar>);

		add("up(%s)",       +[](MyInput x) -> Vector { return up});
		add("down(%s)",     +[](MyInput x) -> Vector { return down});

		add("speaker(%s)",       +[](MyInput x) -> Object { return x.scene.speaker; });
		add("figure(%s)",        +[](MyInput x) -> Object { return x.scene.figure; });
		add("ground(%s)",        +[](MyInput x) -> Object { return x.scene.ground; });
		add("x",             Builtins::X<MyGrammar>);
	}
} grammar;
