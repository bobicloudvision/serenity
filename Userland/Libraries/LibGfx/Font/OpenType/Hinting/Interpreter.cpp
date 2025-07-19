/*
 * Copyright (c) 2023, SerenityOS developers.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/Math.h>
#include <LibGfx/Font/OpenType/Hinting/Interpreter.h>

namespace OpenType::Hinting {

void BasicInterpreter::reset_for_glyph(Vector<Gfx::FloatPoint> const& points)
{
    m_glyph_zone.resize(points.size());
    for (size_t i = 0; i < points.size(); ++i) {
        m_glyph_zone.current_points[i] = points[i];
        m_glyph_zone.original_points[i] = points[i];
        m_glyph_zone.touched_x[i] = false;
        m_glyph_zone.touched_y[i] = false;
    }
    
    // Reset graphics state to defaults
    m_gs = {};
    
    // Clear stack and allocate basic storage
    m_stack.clear();
    m_storage.resize(64); // Basic storage allocation
    m_cvt.resize(16);     // Basic CVT allocation
}

void BasicInterpreter::execute_program(ReadonlyBytes program)
{
    if (program.is_empty())
        return;
    
    InstructionStream stream(*this, program);
    while (!stream.at_end()) {
        stream.process_next_instruction();
    }
}

void BasicInterpreter::default_handler(Context)
{
    // For unimplemented instructions, just do nothing
    // This allows basic functionality even without full implementation
}

i32 BasicInterpreter::pop()
{
    if (m_stack.is_empty())
        return 0;
    return m_stack.take_last();
}

float BasicInterpreter::round_value(float value) const
{
    switch (m_gs.round_state) {
    case GraphicsState::RoundState::ToGrid:
        return roundf(value);
    case GraphicsState::RoundState::ToHalfGrid:
        return roundf(value - 0.5f) + 0.5f;
    case GraphicsState::RoundState::ToDoubleGrid:
        return roundf(value * 2.0f) / 2.0f;
    case GraphicsState::RoundState::UpToGrid:
        return ceilf(value);
    case GraphicsState::RoundState::DownToGrid:
        return floorf(value);
    case GraphicsState::RoundState::Off:
        return value;
    }
    return value;
}

void BasicInterpreter::move_point(u32 point_index, float distance)
{
    if (point_index >= m_glyph_zone.current_points.size())
        return;
    
    Gfx::FloatPoint& point = m_glyph_zone.current_points[point_index];
    point = point + m_gs.freedom_vector * distance;
    
    // Mark point as touched
    if (AK::abs(m_gs.freedom_vector.x()) > 0.5f)
        m_glyph_zone.touched_x[point_index] = true;
    if (AK::abs(m_gs.freedom_vector.y()) > 0.5f)
        m_glyph_zone.touched_y[point_index] = true;
}

float BasicInterpreter::project_vector(Gfx::FloatPoint const& vec) const
{
    return vec.x() * m_gs.projection_vector.x() + vec.y() * m_gs.projection_vector.y();
}

Zone& BasicInterpreter::get_zone(u32 zone_pointer)
{
    return zone_pointer == 0 ? m_twilight_zone : m_glyph_zone;
}

Gfx::FloatPoint BasicInterpreter::get_unit_vector(Gfx::FloatPoint const& p1, Gfx::FloatPoint const& p2) const
{
    Gfx::FloatPoint diff = p2 - p1;
    float length = sqrtf(diff.x() * diff.x() + diff.y() * diff.y());
    if (length == 0.0f)
        return { 1.0f, 0.0f };
    return { diff.x() / length, diff.y() / length };
}

// Set freedom and projection Vectors to Coordinate Axis
void BasicInterpreter::handle_SVTCA(Context context)
{
    bool y_axis = context.instruction().a(); // bit 0 determines axis
    if (y_axis) {
        m_gs.freedom_vector = { 0.0f, 1.0f };
        m_gs.projection_vector = { 0.0f, 1.0f };
        m_gs.dual_projection_vector = { 0.0f, 1.0f };
    } else {
        m_gs.freedom_vector = { 1.0f, 0.0f };
        m_gs.projection_vector = { 1.0f, 0.0f };
        m_gs.dual_projection_vector = { 1.0f, 0.0f };
    }
}

// Set Projection Vector to Coordinate Axis
void BasicInterpreter::handle_SPVTCA(Context context)
{
    bool y_axis = context.instruction().a();
    if (y_axis) {
        m_gs.projection_vector = { 0.0f, 1.0f };
        m_gs.dual_projection_vector = { 0.0f, 1.0f };
    } else {
        m_gs.projection_vector = { 1.0f, 0.0f };
        m_gs.dual_projection_vector = { 1.0f, 0.0f };
    }
}

// Set Freedom Vector to Coordinate Axis
void BasicInterpreter::handle_SFVTCA(Context context)
{
    bool y_axis = context.instruction().a();
    m_gs.freedom_vector = y_axis ? Gfx::FloatPoint(0.0f, 1.0f) : Gfx::FloatPoint(1.0f, 0.0f);
}

// Set Reference Point 0
void BasicInterpreter::handle_SRP0(Context)
{
    m_gs.rp0 = static_cast<u32>(pop());
}

// Set Reference Point 1  
void BasicInterpreter::handle_SRP1(Context)
{
    m_gs.rp1 = static_cast<u32>(pop());
}

// Set Reference Point 2
void BasicInterpreter::handle_SRP2(Context)
{
    m_gs.rp2 = static_cast<u32>(pop());
}

// Set Zone PointerS
void BasicInterpreter::handle_SZPS(Context)
{
    u32 zone = static_cast<u32>(pop());
    m_gs.zp0 = zone;
    m_gs.zp1 = zone;
    m_gs.zp2 = zone;
}

// Round To Half Grid
void BasicInterpreter::handle_RTHG(Context)
{
    m_gs.round_state = GraphicsState::RoundState::ToHalfGrid;
}

// Round To Grid
void BasicInterpreter::handle_RTG(Context)
{
    m_gs.round_state = GraphicsState::RoundState::ToGrid;
}

// Round To Double Grid
void BasicInterpreter::handle_RTDG(Context)
{
    m_gs.round_state = GraphicsState::RoundState::ToDoubleGrid;
}

// Move Direct Absolute Point
void BasicInterpreter::handle_MDAP(Context context)
{
    u32 point_index = static_cast<u32>(pop());
    bool round = context.instruction().a();
    
    if (point_index >= m_glyph_zone.current_points.size())
        return;
    
    Gfx::FloatPoint& point = m_glyph_zone.current_points[point_index];
    
    if (round) {
        float projected = project_vector(point);
        float rounded = round_value(projected);
        float distance = rounded - projected;
        move_point(point_index, distance);
    }
    
    m_gs.rp0 = point_index;
    m_gs.rp1 = point_index;
}

// Move Indirect Absolute Point
void BasicInterpreter::handle_MIAP(Context context)
{
    u32 cvt_index = static_cast<u32>(pop());
    u32 point_index = static_cast<u32>(pop());
    bool round = context.instruction().a();
    
    if (point_index >= m_glyph_zone.current_points.size() || cvt_index >= m_cvt.size())
        return;
    
    Gfx::FloatPoint& point = m_glyph_zone.current_points[point_index];
    float cvt_value = m_cvt[cvt_index];
    float projected = project_vector(point);
    
    float distance;
    if (round) {
        distance = round_value(cvt_value) - projected;
    } else {
        distance = cvt_value - projected;
    }
    
    move_point(point_index, distance);
    m_gs.rp0 = point_index;
    m_gs.rp1 = point_index;
}

// Move Direct Relative Point
void BasicInterpreter::handle_MDRP(Context context)
{
    u32 point_index = static_cast<u32>(pop());
    
    if (point_index >= m_glyph_zone.current_points.size() || m_gs.rp0 >= m_glyph_zone.current_points.size())
        return;
    
    Gfx::FloatPoint& point = m_glyph_zone.current_points[point_index];
    Gfx::FloatPoint& rp0_point = m_glyph_zone.current_points[m_gs.rp0];
    
    // Calculate original distance
    Gfx::FloatPoint original_diff = m_glyph_zone.original_points[point_index] - m_glyph_zone.original_points[m_gs.rp0];
    float original_distance = project_vector(original_diff);
    
    // Apply minimum distance if needed
    if (AK::abs(original_distance) < m_gs.minimum_distance) {
        original_distance = original_distance >= 0 ? m_gs.minimum_distance : -m_gs.minimum_distance;
    }
    
    // Round if requested
    if (context.instruction().c()) { // Bit 2 is round flag
        original_distance = round_value(original_distance);
    }
    
    // Calculate current distance and move point
    Gfx::FloatPoint current_diff = point - rp0_point;
    float current_distance = project_vector(current_diff);
    float move_distance = original_distance - current_distance;
    
    move_point(point_index, move_distance);
    m_gs.rp1 = m_gs.rp0;
    m_gs.rp2 = point_index;
    m_gs.rp0 = point_index;
}

// Move Indirect Relative Point
void BasicInterpreter::handle_MIRP(Context context)
{
    u32 cvt_index = static_cast<u32>(pop());
    u32 point_index = static_cast<u32>(pop());
    
    if (point_index >= m_glyph_zone.current_points.size() || 
        m_gs.rp0 >= m_glyph_zone.current_points.size() ||
        cvt_index >= m_cvt.size())
        return;
    
    Gfx::FloatPoint& point = m_glyph_zone.current_points[point_index];
    Gfx::FloatPoint& rp0_point = m_glyph_zone.current_points[m_gs.rp0];
    float cvt_distance = m_cvt[cvt_index];
    
    // Round if requested
    if (context.instruction().c()) {
        cvt_distance = round_value(cvt_distance);
    }
    
    // Calculate current distance and move point
    Gfx::FloatPoint current_diff = point - rp0_point;
    float current_distance = project_vector(current_diff);
    float move_distance = cvt_distance - current_distance;
    
    move_point(point_index, move_distance);
    m_gs.rp1 = m_gs.rp0;
    m_gs.rp2 = point_index;  
    m_gs.rp0 = point_index;
}

// Interpolate Untouched Points
void BasicInterpreter::handle_IUP(Context context)
{
    bool y_axis = context.instruction().a();
    
    // This is a simplified IUP - a full implementation would be more complex
    // For now, just mark this as a hint that interpolation happened
    for (size_t i = 0; i < m_glyph_zone.current_points.size(); ++i) {
        if (y_axis && !m_glyph_zone.touched_y[i]) {
            // Basic interpolation logic would go here
        } else if (!y_axis && !m_glyph_zone.touched_x[i]) {
            // Basic interpolation logic would go here  
        }
    }
}

// Stack operations
void BasicInterpreter::handle_PUSHB(Context context)
{
    auto values = context.instruction().values();
    for (auto byte : values) {
        push(static_cast<i32>(byte));
    }
}

void BasicInterpreter::handle_PUSHW(Context context)
{
    auto values = context.instruction().values();
    for (size_t i = 0; i < values.size(); i += 2) {
        if (i + 1 < values.size()) {
            i16 word = (static_cast<i16>(values[i]) << 8) | values[i + 1];
            push(static_cast<i32>(word));
        }
    }
}

void BasicInterpreter::handle_NPUSHB(Context context)
{
    handle_PUSHB(context);
}

void BasicInterpreter::handle_NPUSHW(Context context)
{
    handle_PUSHW(context);
}

void BasicInterpreter::handle_POP(Context)
{
    pop();
}

void BasicInterpreter::handle_CLEAR(Context)
{
    m_stack.clear();
}

void BasicInterpreter::handle_DUP(Context)
{
    if (!m_stack.is_empty()) {
        push(m_stack.last());
    }
}

void BasicInterpreter::handle_SWAP(Context)
{
    if (m_stack.size() >= 2) {
        auto a = pop();
        auto b = pop(); 
        push(a);
        push(b);
    }
}

} 