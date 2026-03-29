# Getting Started with Karm

This guide will help you set up a new project using the Karm framework. Follow the steps below to get started with building your first Karm application.

## 1. Install UV

Before starting, you need to have **uv** installed on your system. It is a fast Python package installer and resolver that Cutekit relies on.

> **Note:** If you haven't installed it yet, follow the official instructions in the [uv installation guide](https://docs.astral.sh/uv/getting-started/installation/).

## 2. Install Cutekit

Once `uv` is installed, use it to install the Cutekit build system globally on your machine:

```bash
uv tool install cutekit
```

## 3. Initialize a New Project

Next, create a new Cutekit project workspace and navigate into it:

```bash
ck init --kind=project example
```

```bash
cd example
```

## 4. Add the Karm Dependency

To use the Karm framework, you need to add it as an external dependency. Open the newly generated **`project.json`** file and add the `skift/karm` repository to the `"extern"` block:

```json
{
    // ... existing configuration ...
    "extern": {
        "skift/karm": {
            "git": "https://codeberg.org/skift/karm.git",
            "tag": "main"
        }
    }
}
```

Save the file, then tell Cutekit to download and install the dependency:

```bash
ck install
```

## 5. Create a Component

Now, initialize a new component (an executable or library) inside your project called `hello-world`, and create the main source file:

```bash
ck init --kind=component hello-world
```

```bash
touch src/hello-world/main.cpp
```

## 6. Write the "Hello World" Code

Open the **`src/hello-world/main.cpp`** file you just created and paste the following Karm framework code into it:

```cpp
#include <karm/entry>

import Karm.Sys;

using namespace Karm;

Async::Task<> entryPointAsync(Sys::Env&, Async::CancellationToken) {
    Sys::println("Hello, world!");
    co_return Ok();
}
```

## 7. Build and Run

Finally, use Cutekit to compile and run your new component:

```bash
ck run hello-world
```

## You should see the output:

```
Hello, world!
```

Congratulations! You've successfully set up a Karm project and run your first application. From here, you can explore the Karm documentation to learn more about its features and how to build more complex applications.
