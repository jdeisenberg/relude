/**
ResultIor is similar to Belt.Result, but it has the ability to collect "non-fatal warning" information
during applicative validation.

E.g. if you are doing applicative validation to construct a User model, you could parse a phone number and
allow it, but collect a warning that certain phone number formats are deprecated.
*/
type t('a, 'e) =
  | IOk('a)
  | IError('e)
  | IBoth('a, 'e);

let pure: 'a => t('a, 'e) = a => IOk(a);

let ok: 'a => t('a, 'e) = pure;

let error: 'e => t('a, 'e) = e => IError(e);

let both: ('a, 'e) => t('a, 'e) = (a, e) => IBoth(a, e);

let isOk: 'a 'e. t('a, 'e) => bool =
  fun
  | IOk(_) => true
  | IError(_) => false
  | IBoth(_, _) => false;

let isError: 'a 'e. t('a, 'e) => bool =
  fun
  | IOk(_) => false
  | IError(_) => true
  | IBoth(_, _) => false;

let isBoth: 'a 'e. t('a, 'e) => bool =
  fun
  | IOk(_) => false
  | IError(_) => false
  | IBoth(_, _) => true;

let map: ('a => 'b, t('a, 'e)) => t('b, 'e) =
  (f, fa) =>
    switch (fa) {
    | IError(_) as e => e
    | IOk(a) => IOk(f(a))
    | IBoth(a, e) => IBoth(f(a), e)
    };

let apply: (t('a => 'b, 'e), t('a, 'e), ('e, 'e) => 'e) => t('b, 'e) =
  (ff, fa, appendErrors) =>
    switch (ff, fa) {
    | (IOk(f), IOk(a)) => IOk(f(a))
    | (IOk(_), IError(e)) => IError(e)
    | (IOk(f), IBoth(a, e)) => IBoth(f(a), e)
    | (IError(e), IOk(_)) => IError(e)
    | (IError(e1), IError(e2)) => IError(appendErrors(e1, e2))
    | (IError(e1), IBoth(_, e2)) => IError(appendErrors(e1, e2))
    | (IBoth(f, e1), IOk(a)) => IBoth(f(a), e1)
    | (IBoth(_, e1), IError(e2)) => IError(appendErrors(e1, e2))
    | (IBoth(f, e1), IBoth(a, e2)) => IBoth(f(a), appendErrors(e1, e2))
    };

let bind: (t('a, 'e), 'a => t('b, 'e)) => t('b, 'e) =
  (fa, f) =>
    switch (fa) {
    | IOk(a) => f(a)
    | IError(_) as err => err
    | IBoth(a, _) => f(a)
    };

let flatMap: ('a => t('b, 'e), t('a, 'e)) => t('b, 'e) =
  (f, fa) => bind(fa, f);

let map2:
  (('a, 'b) => 'c, t('a, 'x), t('b, 'x), ('x, 'x) => 'x) => t('c, 'x) =
  (f, fa, fb, appendErrors) => apply(map(f, fa), fb, appendErrors);

let map3:
  (('a, 'b, 'c) => 'd, t('a, 'x), t('b, 'x), t('c, 'x), ('x, 'x) => 'x) =>
  t('d, 'x) =
  (f, fa, fb, fc, appendErrors) =>
    apply(map2(f, fa, fb, appendErrors), fc, appendErrors);

let map4:
  (
    ('a, 'b, 'c, 'd) => 'e,
    t('a, 'x),
    t('b, 'x),
    t('c, 'x),
    t('d, 'x),
    ('x, 'x) => 'x
  ) =>
  t('e, 'x) =
  (f, fa, fb, fc, fd, appendErrors) =>
    apply(map3(f, fa, fb, fc, appendErrors), fd, appendErrors);

let map5:
  (
    ('a, 'b, 'c, 'd, 'e) => 'f,
    t('a, 'x),
    t('b, 'x),
    t('c, 'x),
    t('d, 'x),
    t('e, 'x),
    ('x, 'x) => 'x
  ) =>
  t('f, 'x) =
  (f, fa, fb, fc, fd, fe, appendErrors) =>
    apply(map4(f, fa, fb, fc, fd, appendErrors), fe, appendErrors);

module type FUNCTOR_F =
  (
    Errors: BsAbstract.Interface.SEMIGROUP_ANY,
    Error: BsAbstract.Interface.TYPE,
  ) =>
   BsAbstract.Interface.FUNCTOR with type t('a) = t('a, Errors.t(Error.t));

module Functor: FUNCTOR_F =
  (
    Errors: BsAbstract.Interface.SEMIGROUP_ANY,
    Error: BsAbstract.Interface.TYPE,
  ) => {
    type nonrec t('a) = t('a, Errors.t(Error.t));
    let map = map;
  };

module type APPLY_F =
  (
    Errors: BsAbstract.Interface.SEMIGROUP_ANY,
    Error: BsAbstract.Interface.TYPE,
  ) =>
   BsAbstract.Interface.APPLY with type t('a) = t('a, Errors.t(Error.t));

module Apply: APPLY_F =
  (
    Errors: BsAbstract.Interface.SEMIGROUP_ANY,
    Error: BsAbstract.Interface.TYPE,
  ) => {
    include Functor(Errors, Error);
    let apply = (ff, fa) => apply(ff, fa, Errors.append);
  };

module type APPLICATIVE_F =
  (
    Errors: BsAbstract.Interface.SEMIGROUP_ANY,
    Error: BsAbstract.Interface.TYPE,
  ) =>

    BsAbstract.Interface.APPLICATIVE with
      type t('a) = t('a, Errors.t(Error.t));

module Applicative: APPLICATIVE_F =
  (
    Errors: BsAbstract.Interface.SEMIGROUP_ANY,
    Error: BsAbstract.Interface.TYPE,
  ) => {
    include Apply(Errors, Error);
    let pure = pure;
  };

module type MONAD_F =
  (
    Errors: BsAbstract.Interface.SEMIGROUP_ANY,
    Error: BsAbstract.Interface.TYPE,
  ) =>
   BsAbstract.Interface.MONAD with type t('a) = t('a, Errors.t(Error.t));

module Monad: MONAD_F =
  (
    Errors: BsAbstract.Interface.SEMIGROUP_ANY,
    Error: BsAbstract.Interface.TYPE,
  ) => {
    include Applicative(Errors, Error);
    let flat_map = bind;
  };

module Infix =
       (
         Errors: BsAbstract.Interface.SEMIGROUP_ANY,
         Error: BsAbstract.Interface.TYPE,
       ) => {
  module Functor = BsAbstract.Infix.Functor((Functor(Errors, Error)));

  module Apply = BsAbstract.Infix.Apply((Apply(Errors, Error)));

  module Monad = BsAbstract.Infix.Monad((Monad(Errors, Error)));
};