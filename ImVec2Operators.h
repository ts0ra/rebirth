#pragma once

#include <imgui.h>

// Overload the + operator
ImVec2 operator+(const ImVec2& lhs, const ImVec2& rhs) {
    return ImVec2(lhs.x + rhs.x, lhs.y + rhs.y);
}

// Overload the - operator
ImVec2 operator-(const ImVec2& lhs, const ImVec2& rhs) {
    return ImVec2(lhs.x - rhs.x, lhs.y - rhs.y);
}

// Overload the * operator (scalar multiplication)
ImVec2 operator*(const ImVec2& lhs, float scalar) {
    return ImVec2(lhs.x * scalar, lhs.y * scalar);
}

// Overload the / operator (scalar division)
ImVec2 operator/(const ImVec2& lhs, float scalar) {
    return ImVec2(lhs.x / scalar, lhs.y / scalar);
}

// Overload the * operator (element-wise multiplication)
ImVec2 operator*(const ImVec2& lhs, const ImVec2& rhs) {
    return ImVec2(lhs.x * rhs.x, lhs.y * rhs.y);
}

// Overload the / operator (element-wise division)
ImVec2 operator/(const ImVec2& lhs, const ImVec2& rhs) {
    return ImVec2(lhs.x / rhs.x, lhs.y / rhs.y);
}
