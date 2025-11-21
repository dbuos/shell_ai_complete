# Shell Complete - LLM-Powered Shell Autocompletion

Intelligent shell command completion and correction using Cerebras.

## Features

- Real-time command completion using Cerebras
- Inline replacement of current command
- Command suggestions and corrections
- Simple C++ implementation with zsh integration

## Requirements

- C++ compiler (g++ or clang++)
- zsh shell
- curl
- Cerebras API key

## Installation

1. Build the program:
```bash
make
```

2. Set your Cerebras API key (Set it on ~/.zshrc):
```bash
export CEREBRAS_API_KEY='your-api-key-here'
```

3. Add to your `~/.zshrc`:
```bash
source /path/to/shell_helper/shell_complete.zsh
```

4. Reload your shell:
```bash
source ~/.zshrc
```

## Usage

### Keyboard Shortcuts

- **Ctrl+Z**: Replace current command line with LLM completion
- **Ctrl+X Ctrl+L**: Show LLM suggestion below prompt

### Examples

Type a partial command and press Ctrl+Space:
```
find all pdf files in cur
```

The LLM will complete it to:
```
find . -name "*.pdf"
```

## How It Works

1. The zsh widget captures your current command line
2. Sends it to the C++ program
3. The program calls Cerebras API (gpt-oss) for intelligent completion
4. The completion is inserted back into your shell

## Cleanup

```bash
make clean
```
