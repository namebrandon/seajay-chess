# SeaJay Chess Engine Development Container
FROM --platform=linux/amd64 ubuntu:22.04

ARG TZ=America/Chicago
ENV TZ=$TZ
ENV DEBIAN_FRONTEND=noninteractive

# Update and install base packages
RUN apt-get update && apt-get install -y \
    # Build essentials
    build-essential \
    cmake \
    ninja-build \
    make \
    ccache \
    pkg-config \
    # Compilers
    gcc-12 \
    g++-12 \
    clang-15 \
    clang-tools-15 \
    clang-format-15 \
    clang-tidy-15 \
    libc++-15-dev \
    libc++abi-15-dev \
    lld-15 \
    llvm-15 \
    # Debugging and profiling
    gdb \
    lldb-15 \
    valgrind \
    linux-tools-generic \
    linux-cloud-tools-generic \
    google-perftools \
    # Static analysis
    cppcheck \
    iwyu \
    # Version control
    git \
    gh \
    # Python and package management
    python3.11 \
    python3.11-dev \
    python3-pip \
    python3-venv \
    # Network and system tools
    curl \
    wget \
    unzip \
    sudo \
    tzdata \
    locales \
    # Development helpers
    htop \
    ncdu \
    tree \
    ripgrep \
    fd-find \
    bat \
    jq \
    # Shell and terminal
    zsh \
    fzf \
    # Editors
    nano \
    vim \
    # Documentation tools
    pandoc \
    doxygen \
    graphviz \
    && rm -rf /var/lib/apt/lists/*

# Set default compilers
RUN update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-12 100 && \
    update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-12 100 && \
    update-alternatives --install /usr/bin/clang clang /usr/bin/clang-15 100 && \
    update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-15 100

# Install Python packages for chess development
RUN pip3 install --upgrade pip && \
    pip3 install \
        python-chess \
        pandas \
        numpy \
        matplotlib \
        scikit-learn \
        tqdm \
        requests \
        tabulate

# Install uv for fast Python package management
RUN curl -LsSf https://astral.sh/uv/install.sh | sh
ENV PATH="/root/.cargo/bin:$PATH"

# Install git-delta from GitHub releases
RUN DELTA_VERSION="0.17.0" && \
    wget -q https://github.com/dandavison/delta/releases/download/${DELTA_VERSION}/git-delta_${DELTA_VERSION}_amd64.deb && \
    dpkg -i git-delta_${DELTA_VERSION}_amd64.deb && \
    rm git-delta_${DELTA_VERSION}_amd64.deb

# Install Node.js (Claude CLI can be installed later if needed)
RUN curl -fsSL https://deb.nodesource.com/setup_20.x | bash - && \
    apt-get install -y nodejs

# Create non-root user
RUN useradd -m -s /bin/zsh -G sudo developer && \
    echo "developer ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers

# Set up zsh for developer user
USER developer
RUN sh -c "$(curl -fsSL https://raw.githubusercontent.com/ohmyzsh/ohmyzsh/master/tools/install.sh)" "" --unattended && \
    git clone https://github.com/zsh-users/zsh-autosuggestions ${ZSH_CUSTOM:-~/.oh-my-zsh/custom}/plugins/zsh-autosuggestions && \
    git clone https://github.com/zsh-users/zsh-syntax-highlighting.git ${ZSH_CUSTOM:-~/.oh-my-zsh/custom}/plugins/zsh-syntax-highlighting && \
    sed -i 's/plugins=(git)/plugins=(git zsh-autosuggestions zsh-syntax-highlighting)/' ~/.zshrc

# Add useful aliases
RUN echo '# Chess engine development aliases' >> ~/.zshrc && \
    echo 'alias rebuild="cd /workspace/build && cmake .. && make -j"' >> ~/.zshrc && \
    echo 'alias perft="/workspace/bin/seajay perft"' >> ~/.zshrc && \
    echo 'alias bench="/workspace/bin/seajay bench"' >> ~/.zshrc && \
    echo 'alias sprt="/workspace/tools/scripts/run-sprt.sh"' >> ~/.zshrc && \
    echo 'alias ll="ls -alF"' >> ~/.zshrc && \
    echo 'alias la="ls -A"' >> ~/.zshrc && \
    echo 'alias l="ls -CF"' >> ~/.zshrc && \
    echo '' >> ~/.zshrc && \
    echo '# Environment variables' >> ~/.zshrc && \
    echo 'export WORKSPACE=/workspace' >> ~/.zshrc && \
    echo 'export EXTERNAL_DIR=/workspace/external' >> ~/.zshrc && \
    echo 'export MAKEFLAGS="-j$(nproc)"' >> ~/.zshrc && \
    echo 'export CLAUDE_CONFIG_DIR="$HOME/.config/claude"' >> ~/.zshrc

# Create necessary directories
RUN mkdir -p /home/developer/.config/claude /home/developer/.ccache

# Set working directory
WORKDIR /workspace

# Switch back to root for container startup
USER root

# Set up locale
RUN locale-gen en_US.UTF-8
ENV LANG=en_US.UTF-8
ENV LANGUAGE=en_US:en
ENV LC_ALL=en_US.UTF-8

# Final user
USER developer