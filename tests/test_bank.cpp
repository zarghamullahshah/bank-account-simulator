// Dependency-free test runner: `make test` (or compile this file directly).
// Prints one line per check and exits non-zero on any failure.
#include <iostream>
#include <string>

#include "../src/bank.h"

using bank::Account;
using bank::Bank;
using bank::Cents;

static int checks_run = 0;
static int checks_failed = 0;

#define CHECK(cond)                                                        \
    do {                                                                   \
        ++checks_run;                                                      \
        if (!(cond)) {                                                     \
            ++checks_failed;                                               \
            std::cerr << "FAIL line " << __LINE__ << ": " #cond "\n";      \
        }                                                                  \
    } while (0)

int main() {
    // --- Account basics ---
    Account acc("A1", "Test User", 10000);  // $100.00
    CHECK(acc.balance_cents() == 10000);
    CHECK(acc.history().size() == 1);  // opening balance recorded
    CHECK(acc.history()[0].type == "deposit");

    // --- Deposit / withdraw ---
    CHECK(acc.deposit(2500, "payday"));
    CHECK(acc.balance_cents() == 12500);
    CHECK(acc.withdraw(500, "coffee"));
    CHECK(acc.balance_cents() == 12000);
    CHECK(!acc.withdraw(12001));          // overdraft refused
    CHECK(acc.balance_cents() == 12000);  // balance unchanged
    CHECK(!acc.deposit(0));
    CHECK(!acc.deposit(-100));
    CHECK(!acc.withdraw(0));
    CHECK(!acc.withdraw(-50));
    CHECK(acc.balance_cents() == 12000);
    CHECK(acc.history().size() == 3);  // opening + deposit + withdraw

    // --- Negative opening balance is clamped to zero ---
    Account broke("A2", "Broke", -500);
    CHECK(broke.balance_cents() == 0);
    CHECK(broke.history().empty());

    // --- Formatting ---
    CHECK(Account::format(0) == "$0.00");
    CHECK(Account::format(5) == "$0.05");
    CHECK(Account::format(105) == "$1.05");
    CHECK(Account::format(123456) == "$1234.56");
    CHECK(Account::format(-250) == "-$2.50");

    // --- Bank: duplicate ids, lookups ---
    Bank bank;
    CHECK(bank.create_account("X1", "Alice", 5000));
    CHECK(!bank.create_account("X1", "Alice again"));  // duplicate refused
    CHECK(bank.account_count() == 1);
    CHECK(bank.find_account("X1") != nullptr);
    CHECK(bank.find_account("missing") == nullptr);
    CHECK(bank.create_account("X2", "Bob", 1000));
    CHECK(bank.account_count() == 2);

    // --- Transfers ---
    CHECK(bank.transfer("X1", "X2", 2000));
    CHECK(bank.find_account("X1")->balance_cents() == 3000);
    CHECK(bank.find_account("X2")->balance_cents() == 3000);
    const auto& x1_hist = bank.find_account("X1")->history();
    const auto& x2_hist = bank.find_account("X2")->history();
    CHECK(x1_hist.back().type == "transfer-out");
    CHECK(x2_hist.back().type == "transfer-in");

    // Failed transfers change nothing.
    CHECK(!bank.transfer("X1", "X2", 3001));   // insufficient funds
    CHECK(!bank.transfer("X1", "X2", 0));
    CHECK(!bank.transfer("X1", "X2", -10));
    CHECK(!bank.transfer("X1", "X1", 100));    // self transfer
    CHECK(!bank.transfer("X1", "NOPE", 100));  // unknown destination
    CHECK(!bank.transfer("NOPE", "X2", 100));  // unknown source
    CHECK(bank.find_account("X1")->balance_cents() == 3000);
    CHECK(bank.find_account("X2")->balance_cents() == 3000);

    std::cout << checks_run << " checks, " << checks_failed << " failed.\n";
    if (checks_failed == 0) std::cout << "All tests passed.\n";
    return checks_failed == 0 ? 0 : 1;
}
