#include <karm/test>

import Karm.Core;
import Karm.Template;

namespace Karm::Template::Tests {

test$("template-eval") {
    auto testCase = [&](Str tmpl, Serde::Object const& self, Str expected) -> Res<> {
        auto res = try$(
            Document::eval(
                tmpl,
                self
            )
        );
        expectEq$(res, expected);
        return Ok();
    };

    // Basic value printing
    try$(testCase(
        "{{.foo}}"s,
        {
            {"foo"s, "bar"s},
        },
        "bar"s
    ));

    // Text with interpolation
    try$(testCase(
        "Hello {{.name}}!"s,
        {
            {"name"s, "World"s},
        },
        "Hello World!"s
    ));

    // Nested field access
    try$(testCase(
        "{{.user.name}}"s,
        Serde::Object{
            {
                "user"s,
                Serde::Object{
                    {"name"s, "Alice"s},
                    {"age"s, 30},
                },
            },
        },
        "Alice"
    ));

    // Simple if statement - true condition
    try$(testCase(
        "{{if .show}}visible{{end}}"s,
        {
            {"show"s, true},
        },
        "visible"s
    ));

    // Simple if statement - false condition
    try$(testCase(
        "{{if .show}}visible{{end}}"s,
        {
            {"show"s, false},
        },
        ""s
    ));

    // If-else statement
    try$(testCase(
        "{{if .loggedIn}}Welcome back{{else}}Please login{{end}}"s,
        {
            {"loggedIn"s, false},
        },
        "Please login"s
    ));

    // If-else if-else chain
    try$(testCase(
        "{{if .role}}admin{{else if .moderator}}mod{{else}}user{{end}}"s,
        {
            {"role"s, false},
            {"moderator"s, true},
        },
        "mod"s
    ));

    // Boolean operators - and
    try$(testCase(
        "{{if .a and .b}}both true{{end}}"s,
        {
            {"a"s, true},
            {"b"s, true},
        },
        "both true"s
    ));

    // Boolean operators - or
    try$(testCase(
        "{{if .a or .b}}at least one{{end}}"s,
        {
            {"a"s, false},
            {"b"s, true},
        },
        "at least one"s
    ));

    // Boolean operators - not
    try$(testCase(
        "{{if not .disabled}}enabled{{end}}"s,
        {
            {"disabled"s, false},
        },
        "enabled"s
    ));

    // For loop - basic
    try$(testCase(
        "{{for item in .items}}{{.item}} {{end}}"s,
        {
            {"items"s, Serde::Array{"a"s, "b"s, "c"s}},
        },
        "a b c "s
    ));

    // For loop - empty array
    try$(testCase(
        "{{for item in .items}}{{.item}}{{end}}"s,
        {
            {"items"s, Serde::Array{}},
        },
        ""
    ));

    // For loop with nested field access
    try$(testCase(
        "{{for user in .users}}{{.user.name}}, {{end}}",
        {
            {
                "users"s,
                Serde::Array{
                    Serde::Object{
                        {"name"s, "Alice"s},
                        {"age"s, 30},
                    },
                    Serde::Object{
                        {"name"s, "Bob"s},
                        {"age"s, 25},
                    },
                },
            },
        },
        "Alice, Bob, "s
    ));

    // Complex nested template
    try$(testCase(
        "Users: {{for user in .users}}{{if .user.active}}{{.user.name}} {{end}}{{end}}",
        {
            {
                "users"s,
                Serde::Array{
                    Serde::Object{
                        {"name"s, "Alice"s},
                        {
                            "active"s,
                            true,
                        },
                    },
                    Serde::Object{
                        {"name"s, "Bob"s},
                        {
                            "active"s,
                            false,
                        },
                    },
                    Serde::Object{
                        {"name"s, "Charlie"s},
                        {
                            "active"s,
                            true,
                        },
                    },
                },
            },
        },
        "Users: Alice Charlie "
    ));

    // Multiple interpolations
    try$(testCase(
        "{{.first}} {{.last}}"s,
        {
            {
                "first"s,
                "John"s,
            },
            {
                "last"s,
                "Doe"s,
            },
        },
        "John Doe"
    ));

    // Literal values in conditions
    try$(testCase(
        "{{if true}}always{{end}}",
        {},
        "always"
    ));
    try$(testCase(
        "{{if false}}never{{else}}fallback{{end}}",
        {},
        "fallback"
    ));

    // Mixed text and expressions
    try$(testCase(
        "Name: {{.name}}, Age: {{.age}}, Active: {{if .active}}Yes{{else}}No{{end}}",
        {
            {"name"s, "Alice"s},
            {"age"s, 30},
            {"active"s, true},
        },
        "Name: Alice, Age: 30, Active: Yes"
    ));

    // Nested loops
    try$(testCase(
        "{{for group in .groups}}{{.group.name}}: {{for item in .group.items}}{{.item}} {{end}}\n{{end}}",
        {
            {
                "groups"s,
                Serde::Array{
                    Serde::Object{
                        {"name"s, "A"s},
                        {"items"s, Serde::Array{"a1"s, "a2"s}},
                    },
                    Serde::Object{
                        {"name"s, "B"s},
                        {"items"s, Serde::Array{"b1"s}},
                    },
                },
            },
        },
        "A: a1 a2 \nB: b1 \n"
    ));

    return Ok();
}

} // namespace Karm::Template::Tests
