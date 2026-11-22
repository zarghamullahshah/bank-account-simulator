# Bank Account Simulator (C++)

A small, dependency-free banking library in C++17: accounts with deposits,
withdrawals, atomic transfers between accounts, and a full transaction
history per account. Money is stored as integer cents, so balances never
suffer floating-point rounding errors.

## What it does

- `Account` — deposit, withdraw, per-account transaction history, `$` formatting
- `Bank` — create accounts, look them up, transfer money atomically
  (a failed transfer changes nothing: insufficient funds, unknown account,
  zero/negative amount, or self-transfer are all refused)
- `src/main.cpp` — a demo that runs a small scripted scenario and prints
  account statements

## How to run

```bash
make        # build the demo
./bank-demo
```

## How to test

```bash
make test   # builds and runs the test suite — 42 checks, no dependencies
```

The suite covers deposits, withdrawals, overdraft refusal, negative and
zero amounts, negative opening balances, money formatting, duplicate
account ids, lookups, successful transfers (with both history legs labelled),
and every failure mode of a transfer.
