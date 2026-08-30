FROM ubuntu:22.04

# Install build dependencies
RUN apt-get update && apt-get install -y \
    g++ \
    make \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Copy the entire project
COPY . /app

# Build the WebSocket server
WORKDIR /app/src
RUN make server

# Expose the WebSocket port
EXPOSE 8080

# Run the server
CMD ["./DominoServer.exe"]
