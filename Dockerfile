# Build stage
FROM ubuntu:22.04 AS builder

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    gcc \
    make \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Copy source files
COPY . .

# Build both server and client
RUN make all

# Create a minimal runtime image for the client
FROM ubuntu:22.04 AS client-runtime

# Install only the necessary runtime dependencies
RUN apt-get update && apt-get install -y \
    libc6 \
    && rm -rf /var/lib/apt/lists/*

# Copy the client executable
COPY --from=builder /app/build/client/werewolf_client /usr/local/bin/werewolf_client

# Set the entrypoint
ENTRYPOINT ["werewolf_client"]

# Create a minimal runtime image for the server
FROM ubuntu:22.04 AS server-runtime

# Install only the necessary runtime dependencies
RUN apt-get update && apt-get install -y \
    libc6 \
    && rm -rf /var/lib/apt/lists/*

# Copy the server executable
COPY --from=builder /app/build/server/werewolf_server /usr/local/bin/werewolf_server

# Set the entrypoint
ENTRYPOINT ["werewolf_server"] 