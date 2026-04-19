// --- Parser API Helpers ---

    // 1. Check if token belongs to a broad category (e.g., token.is<token::Literal>())
    template <typename T>
    [[nodiscard]] bool is() const noexcept {
        return std::holds_alternative<T>(m_value);
    }

    // 2. Check for a specific sub-type (e.g., token.is(token::Operator::Plus))
    template <typename T>
    requires std::is_enum_v<T>
    [[nodiscard]] bool is(T exact_val) const noexcept {
        return is<T>() && std::get<T>(m_value) == exact_val;
    }

    // 3. Extract payload safely
    template <typename T>
    [[nodiscard]] const T& get() const {
        return std::get<T>(m_value);
    }

    // 4. Assert and extract (throws or panics if mismatch)
    template <typename T>
    const T& expect() const {
        if (!is<T>()) {
            throw std::runtime_error(std::format("Unexpected token at {}:{}", m_loc.line, m_loc.column));
            // In the final compiler, this will tie into the Diagnostic Builder instead of throwing
        }
        return get<T>();
    }

    template <typename T>
    requires std::is_enum_v<T>
    void expect(T exact_val) const {
        if (!is(exact_val)) {
            throw std::runtime_error(std::format("Unexpected exact token at {}:{}", m_loc.line, m_loc.column));
        }
    }

// Note: ensure, that extraction of identifier permits only cyrilic characters
// Note: error tokens must be handled (errors)
// Next: test lexer