// Copyright (c) John A. Carlos Jr., all rights reserved.

#if 0

  #define Defer( code ) \
    auto NAMEJOIN( deferred_, __LINE__ ) = _MakeDefer( [&]() { code } )

  Templ struct
  _defer_t
  {
    T lambda;

    ForceInl _defer_t( T t ) :
      lambda( t )
    {
    }

    ForceInl ~_defer_t()
    {
      lambda();
    }
  };

  Templ ForceInl _defer_t<T>
  _MakeDefer( T t )
  {
    return _defer_t<T>( t );
  }

#endif



#if 0

  Templ struct
  ExitScope
  {
    T lambda;
    ExitScope(T t) :
      lambda( t )
    {
    }

    ~ExitScope()
    {
      lambda();
    }

    ExitScope( const ExitScope& );

  private:
    ExitScope& operator=( const ExitScope& );
  };

  struct
  ExitScopeHelp
  {
    Templ ForceInl
    ExitScope<T> operator+(T t)
    {
      return t;
    }
  };

  #define Defer \
    const auto& NAMEJOIN( defer__, __LINE__ ) = ExitScopeHelp() + [&]()

#endif
