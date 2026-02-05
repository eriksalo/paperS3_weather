Import("env")
import os

env_file = os.path.join(env.subst("$PROJECT_DIR"), ".env")

try:
    with open(env_file) as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#") or "=" not in line:
                continue
            key, value = line.split("=", 1)
            key = key.strip()
            value = value.strip().strip('"').strip("'")
            env.Append(CPPDEFINES=[(key, env.StringifyMacro(value))])
except FileNotFoundError:
    print("ERROR: .env file not found!")
    print("Copy .env.example to .env and fill in your credentials")
    env.Exit(1)
