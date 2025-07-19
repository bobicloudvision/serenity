/*
 * Copyright (c) 2023, SerenityOS developers.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Vector.h>
#include <LibGfx/Font/OpenType/Hinting/Opcodes.h>
#include <LibGfx/Point.h>

namespace OpenType::Hinting {

struct GraphicsState {
    // Freedom vector - direction along which points can move
    Gfx::FloatPoint freedom_vector { 1.0f, 0.0f };
    
    // Projection vector - direction along which distances are measured
    Gfx::FloatPoint projection_vector { 1.0f, 0.0f };
    
    // Dual projection vector - for measuring distances in the original outline
    Gfx::FloatPoint dual_projection_vector { 1.0f, 0.0f };
    
    // Reference points
    u32 rp0 { 0 };  // Reference point 0
    u32 rp1 { 0 };  // Reference point 1  
    u32 rp2 { 0 };  // Reference point 2
    
    // Zone pointers
    u32 zp0 { 1 };  // Zone pointer 0
    u32 zp1 { 1 };  // Zone pointer 1
    u32 zp2 { 1 };  // Zone pointer 2
    
    // Loop counter for instructions that repeat
    u32 loop_counter { 1 };
    
    // Minimum distance
    float minimum_distance { 1.0f };
    
    // Control value cut-in
    float control_value_cut_in { 17.0f / 16.0f };
    
    // Single width cut-in
    float single_width_cut_in { 0.0f };
    
    // Single width value
    float single_width_value { 0.0f };
    
    // Delta base and shift
    u32 delta_base { 9 };
    u32 delta_shift { 3 };
    
    // Rounding state
    enum class RoundState {
        ToGrid,
        ToHalfGrid,
        ToDoubleGrid,
        UpToGrid,
        DownToGrid,
        Off
    } round_state { RoundState::ToGrid };
    
    // Auto flip
    bool auto_flip { true };
    
    // Instruction control
    bool inhibit_grid_fitting { false };
};

struct Zone {
    Vector<Gfx::FloatPoint> current_points;
    Vector<Gfx::FloatPoint> original_points;
    Vector<bool> touched_x;
    Vector<bool> touched_y;
    
    void resize(size_t point_count) {
        current_points.resize(point_count);
        original_points.resize(point_count);
        touched_x.resize(point_count);
        touched_y.resize(point_count);
    }
};

class BasicInterpreter : public InstructionHandler {
public:
    BasicInterpreter() = default;
    
    void reset_for_glyph(Vector<Gfx::FloatPoint> const& points);
    void execute_program(ReadonlyBytes program);
    Vector<Gfx::FloatPoint> const& get_hinted_points() const { return m_glyph_zone.current_points; }
    
    // InstructionHandler implementation
    virtual void default_handler(Context context) override;
    
    // Implement key instructions for basic hinting
    virtual void handle_SVTCA(Context context) override;
    virtual void handle_SPVTCA(Context context) override; 
    virtual void handle_SFVTCA(Context context) override;
    virtual void handle_SRP0(Context context) override;
    virtual void handle_SRP1(Context context) override;
    virtual void handle_SRP2(Context context) override;
    virtual void handle_SZPS(Context context) override;
    virtual void handle_RTHG(Context context) override;
    virtual void handle_RTG(Context context) override;
    virtual void handle_RTDG(Context context) override;
    virtual void handle_MDAP(Context context) override;
    virtual void handle_MIAP(Context context) override;
    virtual void handle_MDRP(Context context) override;
    virtual void handle_MIRP(Context context) override;
    virtual void handle_IUP(Context context) override;
    
    // Stack operations
    virtual void handle_PUSHB(Context context) override;
    virtual void handle_PUSHW(Context context) override;
    virtual void handle_NPUSHB(Context context) override;
    virtual void handle_NPUSHW(Context context) override;
    virtual void handle_POP(Context context) override;
    virtual void handle_CLEAR(Context context) override;
    virtual void handle_DUP(Context context) override;
    virtual void handle_SWAP(Context context) override;

private:
    GraphicsState m_gs;
    Zone m_glyph_zone;
    Zone m_twilight_zone;
    Vector<i32> m_stack;
    Vector<i32> m_storage;
    Vector<i32> m_cvt;
    
    void push(i32 value) { m_stack.append(value); }
    i32 pop();
    
    float round_value(float value) const;
    void move_point(u32 point_index, float distance);
    float project_vector(Gfx::FloatPoint const& vec) const;
    
    Zone& get_zone(u32 zone_pointer);
    Gfx::FloatPoint get_unit_vector(Gfx::FloatPoint const& p1, Gfx::FloatPoint const& p2) const;
};

} 