# SquareDesk MCP Server

This is a prototype MCP (Model Context Protocol) server for SquareDesk that will eventually expose square dance simulation features from the SD engine.

## Building as Claude Desktop Extension (DXT)

The easiest way to build and install the MCP server is as a Desktop Extension for Claude Desktop:

```bash
# From the sd_mcp directory, run the build script
./bin/build_dxt.sh
```

This will:
1. Compile the MCP server from `src/main_simple.cpp`
2. Create the DXT package at `build/squaredesk-mcp.dxt`
3. Make it ready for installation in Claude Desktop

To install the extension, simply double-click on the generated `build/squaredesk-mcp.dxt` file.

## Alternative Build (Standalone)

For development or standalone testing, you can also build manually:

```bash
# Compile the stdio-based MCP server
g++ -std=c++17 -O2 -o build/bin/sd_mcp src/main_simple.cpp
chmod +x build/bin/sd_mcp
```

## Current Features

- Echo tool: A simple tool that echoes back the provided text with a prefix
- Full JSON-RPC 2.0 compliance with proper notification handling
- Stdio transport for Claude Desktop integration

## Testing the MCP Server

When built as a DXT and installed in Claude Desktop, you can test the echo tool directly in Claude by asking:

"Use the echo tool to repeat 'Hello from SquareDesk!'"

For standalone testing with JSON input:

```bash
# Test initialize
echo '{"jsonrpc": "2.0", "method": "initialize", "id": 1}' | ./build/bin/sd_mcp

# Test tools/list
echo '{"jsonrpc": "2.0", "method": "tools/list", "id": 2}' | ./build/bin/sd_mcp

# Test echo tool
echo '{"jsonrpc": "2.0", "method": "tools/call", "params": {"name": "echo", "arguments": {"text": "Hello!"}}, "id": 3}' | ./build/bin/sd_mcp
```

## Integration with SquareDesk

To integrate with the main SquareDesk build process, you can invoke the DXT build from your qmake project:

```qmake
# In your .pro file
system(cd sd_mcp && ./bin/build_dxt.sh)
```

## Next Steps

1. Add SD engine integration
2. Implement square dance simulation tools
3. Add more comprehensive error handling
4. Consider adding authentication/security features