// Copyright (c) John A. Carlos Jr., all rights reserved.

struct
listview_dblclick_t
{
  bool first_made;
  idx_t first_cursor;
  u64 first_clock;
};
struct
listview_rect_t
{
  rectf32_t rect;
  idx_t row_idx;
};
struct
listview_t
{
  // publicly-accessed
  idx_t cursor; // index into matches. commonly read by the owner/caller.

  // not publicly-accessed
  idx_t* len; // number of items in the view, indirect so we can share it with different backing stores.
  idx_t scroll_target; // desired row index at the CENTER of view
  f64 scroll_start; // current row index at the TOP of view
  kahansum64_t scroll_vel;
  idx_t scroll_end; // current row index at the BOTTOM of view
  idx_t window_n_lines;
  idx_t pageupdn_distance;
  bool scroll_grabbed;
  bool has_scrollbar;
  listview_dblclick_t dblclick;
  stack_resizeable_cont_t<listview_rect_t> rowrects;
  rectf32_t scroll_track;
  rectf32_t scroll_btn_up;
  rectf32_t scroll_btn_dn;
  rectf32_t scroll_btn_pos;
  rectf32_t scroll_bounds_view_and_bar;
  rectf32_t scroll_bounds_view;
};

Inl void
Init( listview_t* lv, idx_t* len )
{
  lv->len = len;
  lv->cursor = 0;
  lv->scroll_start = 0;
  lv->scroll_vel = {};
  lv->scroll_target = 0;
  lv->scroll_end = 0;
  lv->window_n_lines = 0;
  lv->pageupdn_distance = 0;
  lv->scroll_grabbed = 0;
  lv->dblclick.first_made = 0;
  lv->dblclick.first_cursor = 0;
  lv->dblclick.first_clock = 0;
  Alloc( lv->rowrects, 128 );
  lv->scroll_track   = {};
  lv->scroll_btn_up  = {};
  lv->scroll_btn_dn  = {};
  lv->scroll_btn_pos = {};
  lv->scroll_bounds_view_and_bar = {};
  lv->scroll_bounds_view = {};
}
Inl void
Kill( listview_t* lv )
{
  lv->len = 0;
  lv->cursor = 0;
  lv->scroll_start = 0;
  lv->scroll_vel = {};
  lv->scroll_target = 0;
  lv->scroll_end = 0;
  lv->window_n_lines = 0;
  lv->pageupdn_distance = 0;
  lv->scroll_grabbed = 0;
  lv->dblclick.first_made = 0;
  lv->dblclick.first_cursor = 0;
  lv->dblclick.first_clock = 0;
  Free( lv->rowrects );
  lv->scroll_track   = {};
  lv->scroll_btn_up  = {};
  lv->scroll_btn_dn  = {};
  lv->scroll_btn_pos = {};
  lv->scroll_bounds_view_and_bar = {};
  lv->scroll_bounds_view = {};
}

Inl void
ListviewResetCS( listview_t* lv )
{
  lv->cursor = 0;
  lv->scroll_target = 0;
}

Inl idx_t
ListviewForceInbounds(
  listview_t* lv,
  idx_t pos
  )
{
  auto r = MIN( pos, MAX( *lv->len, 1 ) - 1 );
  return r;
}

Inl void
ListviewFixupCS( listview_t* lv )
{
  lv->cursor = ListviewForceInbounds( lv, lv->cursor );
  lv->scroll_target = ListviewForceInbounds( lv, lv->scroll_target );
}

Inl idx_t
IdxFromFloatPos(
  listview_t* lv,
  f64 pos
  )
{
  auto v = MAX( pos, 0.0 );
  auto r = ListviewForceInbounds( lv, Cast( idx_t, v ) );
  return r;
}

Inl void
ListviewMakeCursorVisible( listview_t* lv )
{
  auto make_cursor_visible_radius = GetPropFromDb( f32, f32_make_cursor_visible_radius );
  auto nlines_radius = Cast( idx_t, make_cursor_visible_radius * lv->window_n_lines );

  // account for theoretical deletions by forcing inbounds here.
  ListviewFixupCS( lv );

  auto scroll_half = IdxFromFloatPos( lv, lv->scroll_start + 0.5 * lv->window_n_lines );
  auto yl = MAX( scroll_half, nlines_radius ) - nlines_radius;
  auto yr = ListviewForceInbounds( lv, scroll_half + nlines_radius );
  if( lv->cursor < yl ) {
    auto dy = yl - lv->cursor;
    scroll_half = MAX( scroll_half, dy ) - dy;
  }
  elif( lv->cursor > yr ) {
    auto dy = lv->cursor - yr;
    scroll_half = ListviewForceInbounds( lv, scroll_half + dy );
  }
  lv->scroll_target = scroll_half;

  ListviewFixupCS( lv );
}

Inl void
ListviewCursorU( listview_t* lv, idx_t misc )
{
  auto nlines = misc ? misc : 1;
  lv->cursor -= MIN( nlines, lv->cursor );
  ListviewMakeCursorVisible( lv );
}
Inl void
ListviewCursorD( listview_t* lv, idx_t misc )
{
  auto nlines = misc ? misc : 1;
  lv->cursor += nlines;
  ListviewFixupCS( lv );
  ListviewMakeCursorVisible( lv );
}
Inl void
ListviewScrollU( listview_t* lv, idx_t misc )
{
  auto nlines = misc ? misc : 1;
  lv->scroll_target -= MIN( nlines, lv->scroll_target );
}
Inl void
ListviewScrollD( listview_t* lv, idx_t misc )
{
  auto nlines = misc ? misc : 1;
  lv->scroll_target += nlines;
  ListviewFixupCS( lv );
}

Inl f32
MapIdxToT(
  listview_t* lv,
  idx_t idx
  )
{
  auto r = CLAMP( Cast( f32, idx ) / Cast( f32, *lv->len ), 0, 1 );
  return r;
}

Inl void
SetScrollPosFraction(
  listview_t* lv,
  f32 t
  )
{
  auto last_idx = MAX( 1, *lv->len ) - 1;
  auto pos = Round_u32_from_f32( t * last_idx );
  AssertCrash( pos <= last_idx );
  lv->scroll_target = pos;
}

Inl vec2<f32>
GetScrollPos( listview_t* lv )
{
  auto scroll_start_idx = IdxFromFloatPos( lv, lv->scroll_start );
  return _vec2<f32>(
    MapIdxToT( lv, scroll_start_idx ),
    MapIdxToT( lv, lv->scroll_end )
    );
}

Enumc( lvlayer_t )
{
  bkgd,
  sel,
  txt,
  scroll_bkgd,
  scroll_btn,

  COUNT
};

// outputs the list entry span that the caller should render.
Inl void
ListviewUpdateScrollingAndRenderScrollbar(
  listview_t* lv,
  bool& target_valid,
  stack_resizeable_cont_t<f32>& stream,
  font_t& font,
  rectf32_t& bounds,
  vec2<f32> zrange,
  f64 timestep,
  tslice_t<listview_rect_t>* lines
  )
{
  *lines = {};

  auto rgba_cursor_bkgd = GetPropFromDb( vec4<f32>, rgba_cursor_bkgd );
  auto rgba_scroll_btn = GetPropFromDb( vec4<f32>, rgba_scroll_btn );
  auto rgba_scroll_bkgd = GetPropFromDb( vec4<f32>, rgba_scroll_bkgd );

  auto line_h = FontLineH( font );
  auto px_space_advance = FontGetAdvance( font, ' ' );

  auto scroll_pct = GetPropFromDb( f32, f32_scroll_pct );
  auto px_scroll = scroll_pct * line_h;

  // leave a small gap on the left/right, so things aren't exactly at the screen edge.
  // esp. important for scrollbars, since window-resize covers some of the active area.
  bounds.p0.x = MIN( bounds.p0.x + 0.25f * px_space_advance, bounds.p1.x );
  bounds.p1.x = MAX( bounds.p1.x - 0.25f * px_space_advance, bounds.p0.x );

  auto nlines_screen_floored = Cast( idx_t, ( bounds.p1.y - bounds.p0.y ) / line_h );
  lv->pageupdn_distance = MAX( 1, nlines_screen_floored / 2 );
  lv->window_n_lines = nlines_screen_floored;

  // vertical scrolling
  auto half_nlines = 0.5 * lv->window_n_lines;
  auto scroll_half = lv->scroll_start + half_nlines; // intentionally not clamped, so distance forcing works.
  auto distance = Cast( f64, lv->scroll_target ) - scroll_half;
#if 0
  // instant scrolling.
  lv->scroll_start = CLAMP(
    lv->scroll_start + distance,
    -half_nlines,
    *lv->len + half_nlines
    );
#elif 0
  // failed attempt at timed interpolation. felt really terrible to use.
  if( lv->scroll_time < 1.0 ) {
    target_valid = 0;
  }
  auto scroll_start_target_anim = Cast( f64, lv->scroll_target ) - half_nlines;
  constant f64 anim_len = 0.2;
  lv->scroll_time = MIN( lv->scroll_time + timestep / anim_len, 1.0 );
  lv->scroll_start = CLAMP(
    lerp( lv->scroll_start_anim, scroll_start_target_anim, lv->scroll_time ),
    -half_nlines,
    *lv->len + half_nlines
    );
  printf( "start_anim=%F, target=%llu, time=%F, start=%F\n", lv->scroll_start_anim, lv->scroll_target, lv->scroll_time, lv->scroll_start );
#else
  // same force calculations as txt has.
  // note: i tried a trivial multistep Euler method here, but it didn't really improve stability.
  // TODO: try other forms of interpolation: midpoint method, RK4, or just explicit soln, which kinda sucks
  //   because of the extra state req'd.
  constant f64 mass = 1.0;
  constant f64 spring_k = 1000.0;
  static f64 friction_k = 2.2 * Sqrt64( mass * spring_k ); // 2 * Sqrt( m * k ) is critical damping, but we want a little overdamping.
  auto force_spring = spring_k * distance;
  auto force_fric = -friction_k * lv->scroll_vel.sum;
  auto force = force_spring + force_fric;
  auto accel = force / mass;
  auto delta_vel = timestep * accel;
  Add( lv->scroll_vel, delta_vel );
  if( ABS( lv->scroll_vel.sum ) <= 0.2 ) {
    // snap to 0 for small velocities to minimize pixel jitter.
    lv->scroll_vel = {};
  }
  if( ABS( lv->scroll_vel.sum ) > 0 ) {
    // invalidate cached target, since we know animation will require a re-render.
    target_valid = 0;
  }
  auto delta_pos = timestep * lv->scroll_vel.sum;
  lv->scroll_start = CLAMP(
    lv->scroll_start + delta_pos,
    -half_nlines,
    *lv->len + half_nlines
    );
#endif
  AssertCrash( lv->scroll_target <= *lv->len );
  auto scroll_start = IdxFromFloatPos( lv, lv->scroll_start );
  lv->scroll_end = MIN( *lv->len, scroll_start + lv->window_n_lines );
  auto nlines_render = MIN( 1 + lv->window_n_lines, *lv->len - scroll_start );

  // render the scrollbar.

  lv->has_scrollbar = ScrollbarVisible( bounds, px_scroll );
  lv->scroll_bounds_view_and_bar = bounds;
  if( lv->has_scrollbar ) {
    auto scroll_pos = GetScrollPos( lv );
    ScrollbarRender(
      stream,
      bounds,
      scroll_pos.x,
      scroll_pos.y,
      GetZ( zrange, lvlayer_t::scroll_bkgd ),
      GetZ( zrange, lvlayer_t::scroll_btn ),
      px_scroll,
      rgba_scroll_bkgd,
      rgba_scroll_btn,
      &lv->scroll_track,
      &lv->scroll_btn_up,
      &lv->scroll_btn_dn,
      &lv->scroll_btn_pos
      );

    bounds.p1.x = MAX( bounds.p1.x - px_scroll, bounds.p0.x );
  }

  lv->scroll_bounds_view = bounds;

  { // draw bkgd
    RenderQuad(
      stream,
      GetPropFromDb( vec4<f32>, rgba_text_bkgd ),
      bounds,
      bounds,
      GetZ( zrange, lvlayer_t::bkgd )
      );
  }

  // shift line rendering down if we're scrolled up past the first one.
  if( nlines_render  &&  lv->scroll_start < 0 ) {
    auto nlines_prefix = Cast( idx_t, -lv->scroll_start );
    bounds.p0.y = MIN( bounds.p0.y + nlines_prefix * line_h, bounds.p1.y );
  }

  // cursor
  {
    if( scroll_start <= lv->cursor  &&  lv->cursor < lv->scroll_end ) {
      auto p0 = bounds.p0 + _vec2( 0.0f, line_h * ( lv->cursor - scroll_start ) );
      auto p1 = _vec2( bounds.p1.x, p0.y + line_h );
      RenderQuad(
        stream,
        rgba_cursor_bkgd,
        p0,
        p1,
        bounds,
        GetZ( zrange, lvlayer_t::sel )
        );
    }
  }

  // rowrects for mouse hit testing.
  {
    Reserve( lv->rowrects, nlines_render );
    lv->rowrects.len = 0;
    For( i, 0, nlines_render ) {
      auto elem_p0 = bounds.p0 + _vec2<f32>( 0, line_h * i );
      auto elem_p1 = _vec2( bounds.p1.x, MIN( bounds.p1.y, elem_p0.y + line_h ) );
      auto rowrect = AddBack( lv->rowrects );
      rowrect->rect = _rect( elem_p0, elem_p1 );
      rowrect->row_idx = scroll_start + i;
      AssertCrash( rowrect->row_idx <= *lv->len );
    }

    *lines = SliceFromArray( lv->rowrects );
  }
}

Inl idx_t
ListviewMapMouseToCursor(
  listview_t* lv,
  vec2<s32> m
  )
{
  auto c_y = IdxFromFloatPos( lv, lv->scroll_start );
  auto mp = _vec2( Cast( f32, m.x ), Cast( f32, m.y ) );
  f32 min_distance = MAX_f32;
  constant f32 epsilon = 0.001f;
  FORLEN( rowrect, i, lv->rowrects )
    if( PtInBox( mp, rowrect->rect.p0, rowrect->rect.p1, epsilon ) ) {
      c_y = rowrect->row_idx;
      break;
    }
    auto distance = DistanceToBox( mp, rowrect->rect.p0, rowrect->rect.p1 );
    if( distance < min_distance ) {
      min_distance = distance;
      c_y = rowrect->row_idx;
    }
  }
  return c_y;
}

void
ListviewControlMouse(
  listview_t* lv,
  bool& target_valid,
  font_t& font,
  rectf32_t bounds,
  glwmouseevent_t type,
  glwmousebtn_t btn,
  vec2<s32> m,
  vec2<s32> raw_delta,
  s32 dwheel,
  bool* double_clicked_on_line
  )
{
  ProfFunc();

  *double_clicked_on_line = 0;

//  auto px_click_correct = _vec2<s8>(); // TODO: mouse control.
  auto scroll_nlines = GetPropFromDb( u8, u8_scroll_nlines );
  auto scroll_sign = GetPropFromDb( s8, s8_scroll_sign );
  auto dblclick_period_sec = GetPropFromDb( f64, f64_dblclick_period_sec );

  auto keymods = GlwKeyModifiersDown();
  bool mod_isdn = AnyDown( keymods );

  auto has_scrollbar = lv->has_scrollbar;
  auto btn_up = lv->scroll_btn_up;
  auto btn_dn = lv->scroll_btn_dn;
  // auto btn_pos = lv->scroll_btn_pos;
  auto scroll_track = lv->scroll_track;
  auto scroll_t_mouse = GetScrollMouseT( m, scroll_track );
  auto bounds_view_and_bar = lv->scroll_bounds_view_and_bar;
  auto bounds_view = lv->scroll_bounds_view;

  if( has_scrollbar ) {

    if( GlwMouseInsideRect( m, btn_up ) ) {
      switch( type ) {
        case glwmouseevent_t::dn: break;
        case glwmouseevent_t::up: {
          auto t = MapIdxToT( lv, lv->scroll_target );
          SetScrollPosFraction( lv, CLAMP( t - 0.1f, 0, 1 ) );
          target_valid = 0;
        } break;
        case glwmouseevent_t::move: break;
        case glwmouseevent_t::wheelmove: break;
        default: UnreachableCrash();
      }
    }
    elif( GlwMouseInsideRect( m, btn_dn ) ) {
      switch( type ) {
        case glwmouseevent_t::dn: break;
        case glwmouseevent_t::up: {
          auto t = MapIdxToT( lv, lv->scroll_target );
          SetScrollPosFraction( lv, CLAMP( t + 0.1f, 0, 1 ) );
          target_valid = 0;
        } break;
        case glwmouseevent_t::move: break;
        case glwmouseevent_t::wheelmove: break;
        default: UnreachableCrash();
      }
    }
    elif( GlwMouseInsideRect( m, scroll_track ) ) {
      // note that btn_pos isn't really a button; it's just a rendering of where we are in the track.
      // it's more complicated + stateful to track a button that changes size as we scroll.
      // much easier to just do the linear mapping of the track, and it also feels nice to not have to
      // click exactly on a tiny button; the whole track works the same way.
      switch( type ) {
        case glwmouseevent_t::dn: {
          lv->scroll_grabbed = 1;
          SetScrollPosFraction( lv, CLAMP( scroll_t_mouse, 0, 1 ) );
          target_valid = 0;
        } break;
        case glwmouseevent_t::up: break;
        case glwmouseevent_t::move: break;
        case glwmouseevent_t::wheelmove: break;
        default: UnreachableCrash();
      }
    }
  }

  if( GlwMouseInsideRect( m, bounds_view ) ) {
    if( type == glwmouseevent_t::dn  &&  btn == glwmousebtn_t::l ) {
      // TODO: do a text_grabbed thing, where we allow multi-selections.
      lv->cursor = ListviewMapMouseToCursor( lv, m );
      bool same_cursor = ( lv->cursor == lv->dblclick.first_cursor );
      bool double_click = ( lv->dblclick.first_made & same_cursor );
      if( double_click ) {
        if( TimeSecFromClocks64( TimeClock() - lv->dblclick.first_clock ) <= dblclick_period_sec ) {
          lv->dblclick.first_made = 0;
          *double_clicked_on_line = 1;
        } else {
          lv->dblclick.first_clock = TimeClock();
        }
      } else {
        lv->dblclick.first_made = 1;
        lv->dblclick.first_clock = TimeClock();
        lv->dblclick.first_cursor = lv->cursor;
      }
      target_valid = 0;
    }
  }

  switch( type ) {
    case glwmouseevent_t::dn: break;

    case glwmouseevent_t::up: {
      if( lv->scroll_grabbed ) {
        lv->scroll_grabbed = 0;
      }
    } break;

    case glwmouseevent_t::move: {
      if( lv->scroll_grabbed ) {
        SetScrollPosFraction( lv, CLAMP( scroll_t_mouse, 0, 1 ) );
        target_valid = 0;
      }
      // TODO: text_grabbed thing.
    } break;

    case glwmouseevent_t::wheelmove: {
      if( GlwMouseInsideRect( m, bounds_view_and_bar )  &&  dwheel  &&  !mod_isdn ) {
        dwheel *= scroll_sign;
        dwheel *= scroll_nlines;
        if( dwheel < 0 ) {
          ListviewScrollU( lv, Cast( idx_t, -dwheel ) );
        } else {
          ListviewScrollD( lv, Cast( idx_t, dwheel ) );
        }
        target_valid = 0;
      }
    } break;

    default: UnreachableCrash();
  }
}
