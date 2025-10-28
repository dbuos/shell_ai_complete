#!/usr/bin/env zsh

# ZSH widget for LLM-powered shell completion
# Installation: source this file in your .zshrc

SHELL_COMPLETE_BIN="${0:a:h}/shell_complete"

_llm_complete_widget() {
    local current_buffer="$BUFFER"
    local cursor_pos="$CURSOR"

    if [[ -z "$current_buffer" ]]; then
        return
    fi

    # Show loading indicator
    zle -R "Thinking..."

    # Call the C++ completion program
    local completion=$("$SHELL_COMPLETE_BIN" "$current_buffer" 2>/dev/null)

    if [[ -n "$completion" ]]; then
        # Replace the buffer with the completion
        BUFFER="$completion"
        # Move cursor to end
        CURSOR=${#BUFFER}
    fi

    # Refresh the line
    zle reset-prompt
}

_llm_suggest_widget() {
    local current_buffer="$BUFFER"
    local cursor_pos="$CURSOR"

    if [[ -z "$current_buffer" ]]; then
        return
    fi

    # Show loading indicator
    zle -R "Getting suggestions..."

    # Call the C++ completion program
    local completion=$("$SHELL_COMPLETE_BIN" "$current_buffer" 2>/dev/null)

    if [[ -n "$completion" ]]; then
        # Show suggestion below the prompt
        echo "\nSuggestion: $completion"
        zle reset-prompt
    fi
}

# Create zsh widgets
zle -N _llm_complete_widget
zle -N _llm_suggest_widget

# Bind to keyboard shortcuts
# Ctrl+Z for inline completion
bindkey '^Z' _llm_complete_widget
# Ctrl+X Ctrl+L for suggestions
bindkey '^X^L' _llm_suggest_widget

echo "LLM shell completion loaded!"
echo "  Ctrl+Z: Replace current line with LLM suggestion"
echo "  Ctrl+X Ctrl+L: Show LLM suggestion below prompt"
