// [License]
// MIT - See LICENSE.md file in the package.


#include <helion/infer.h>
#include <stdexcept>

using namespace helion;




// find the real type for some type. Basically expands the type
// variable into its real named variable
iir::type *infer::find(iir::type *t) {
  if (t->is_var()) {
    auto tv = t->as_var();
    if (tv->points_to != tv) {
      tv->points_to = find(tv->points_to);
      return tv->points_to;
    }
  }
  return t;
}


static bool is_arrow(iir::type *t) {
  if (!t->is_named()) return false;
  auto n = t->as_named();
  return n->name == "->";
}


static bool occurs(iir::var_type *ta, iir::type *tb) {
  auto t = infer::find(tb);
  if (t->is_var()) return *ta == *tb;
  if (t->is_named()) {
    auto n = t->as_named();
    for (auto &p : n->params) {
      if (occurs(ta, p)) return true;
    }
  }
  return false;
}


void infer::do_union(iir::var_type *ta, iir::type *tb) {
  if (*ta == *tb) return;

  if (occurs(ta, tb)) {
    throw std::logic_error("no rec type please");
  }
  ta->points_to = tb;
}


void infer::unify(iir::type *ta, iir::type *tb) {

  // drop the types to their lowest forms
  auto t1 = find(ta);
  auto t2 = find(tb);

  if (is_arrow(t1) && is_arrow(t2)) {
    auto f1 = t1->as_named();
    auto f2 = t2->as_named();

    auto a1 = f1->params[0];
    auto b1 = f1->params[1];

    auto a2 = f2->params[0];
    auto b2 = f2->params[1];


    unify(a1, a2);
    unify(b1, b2);

    // unify all the types in the methods
    return;
  }


  // unify two named types of the same name
  if (t1->is_named() && t2->is_named()) {
    auto n1 = t1->as_named();
    auto n2 = t2->as_named();
    if (n1->name != n2->name || n1->params.size() != n2->params.size()) {
      std::string err;
      err += "unify error, type mismatch: ";
      err += t1->str();
      err += " and ";
      err += t2->str();
      throw std::logic_error(err);
    }

    for (int i = 0; i < n1->params.size(); i++) {
      unify(n1->params[i], n2->params[i]);
    }
    return;
  }

  if (t1->is_var()) {
    auto tv = t1->as_var();
    do_union(tv, tb);
    return;
  }

  if (t2->is_var()) {
    auto tv = t2->as_var();
    do_union(tv, ta);
    return;
  }

  if (*t1 != *t2) throw std::logic_error("unify error, type mismatch");
};


/*
def Inst(t: Type, ctx: Context): Type = {
  val bindVar = ctx.values.toSet
  val gen = new collection.mutable.HashMap[TVar, TVar]
    def reify(t: Type): Type = { // Try to iterate all free TVar of the type
      Find(t) match {
        case tv: TVar => {
          if (!bindVar.contains(tv)) gen.getOrElseUpdate(tv, MakeFreshTVar())
          else tv
        }
        case Arrow(t1, t2) => Arrow(reify(t1), reify(t2))
        case _ => t
      }
    }
  reify(t)
}
*/



/*
static auto reify(iir::type *t, infer::context ctx,
                  std::unordered_map<iir::type&, iir::type&> &gen)
    -> iir::type * {
  auto found = infer::find(t);
  if (found->is_var()) {
    auto tv = found->as_var();


    auto begin = ctx.begin();

    auto end = ctx.end();

    for (auto it = begin; it != end; it++) {
      if (*(*it).second == *t) return tv;
    }

    if (gen.count(tv) == 0) {
      auto v = &iir::new_variable_type();
      gen.insert(tv, v);
      return v;
    }
    return &gen[*tv];
    // If it is a polytype (generic type), that is, not bind by the context
    // create a new Type variable to be subst free.
  }

  if (is_arrow(t)) {
    auto arr = t->as_named();
    std::vector<iir::type *> np;
    for (auto &p : arr->params) np.push_back(reify(p, ctx, gen));
    return gc::make_collected<iir::named_type>("->", np);
  }

  return t;
}

*/

iir::type *infer::inst(iir::type *t, context &ctx) {
  return t;
  // auto gen = std::unordered_map<iir::type&, iir::type&>();
  //return reify(t, ctx, gen);
}
