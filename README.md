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
- Monthly statements — every deposit and withdrawal in a month in date
  order with a running balance, plus opening balance, totals and closing
  balance (see below)
- Monthly interest — `apply_monthly_interest(rate)` credits the balance
  times the monthly rate (rounded to the nearest cent) as an "interest"
  transaction; refused on a zero balance, a non-positive rate, or interest
  that rounds to zero cents
- `src/main.cpp` — a demo that runs a small scripted scenario and prints
  account statements

## Monthly statements

```bash
./bank-demo statement                 # this month, account ACC-1001
./bank-demo statement ACC-1002        # this month, another account
./bank-demo statement 2026-03         # March 2026, account ACC-1001
./bank-demo statement ACC-1002 2026-03
```

Every transaction carries its real date (dated when it happens, or an
explicit date passed to `deposit`/`withdraw`/`transfer`), so a statement
for any month lists that month's entries in date order with the running
balance after each one. Transfers count as a withdrawal on the sending
account and a deposit on the receiving one. A month with no transactions
still prints its opening balance (carried over from earlier months) and
zero totals.

## How to run

```bash
make        # build the demo
./bank-demo
```

## How to test

```bash
make test   # builds and runs the test suite — 121 checks, no dependencies
```

The suite covers deposits, withdrawals, overdraft refusal, negative and
zero amounts, negative opening balances, money formatting, duplicate
account ids, lookups, successful transfers (with both history legs labelled),
and every failure mode of a transfer — plus monthly statements: several
transactions in one month (including an out-of-order entry), a month with
no transactions, an opening balance carried over from a prior month,
transfers counted as in/out, today's date stamped on undated transactions,
and an invalid month returning an empty statement — plus monthly interest:
credit and compounding, rounding to the nearest cent, refusal on a zero
balance, a non-positive rate or dust that rounds to zero, and interest
appearing in the monthly statement as money in.
