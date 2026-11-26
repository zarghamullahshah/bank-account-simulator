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

    // --- Monthly statements: several transactions, out-of-order entry ---
    {
        Account acc("S1", "Statement User", 0);
        CHECK(acc.deposit(10000, "salary", "deposit", "2026-03-05"));
        CHECK(acc.withdraw(2500, "rent", "withdraw", "2026-03-10"));
        CHECK(acc.deposit(5000, "refund", "deposit", "2026-03-20"));
        // Dated earlier but recorded last: entries must still come out sorted.
        CHECK(acc.deposit(1000, "late entry", "deposit", "2026-03-02"));
        const auto stmt = acc.monthly_statement(2026, 3);
        CHECK(stmt.opening_balance_cents == 0);
        CHECK(stmt.total_deposits_cents == 16000);
        CHECK(stmt.total_withdrawals_cents == 2500);
        CHECK(stmt.closing_balance_cents == 13500);
        CHECK(stmt.entries.size() == 4);
        CHECK(stmt.entries[0].date == "2026-03-02");
        CHECK(stmt.entries[0].balance_after_cents == 1000);
        CHECK(stmt.entries[1].date == "2026-03-05");
        CHECK(stmt.entries[1].balance_after_cents == 11000);
        CHECK(stmt.entries[2].date == "2026-03-10");
        CHECK(stmt.entries[2].amount_cents == -2500);  // signed: money out
        CHECK(stmt.entries[2].balance_after_cents == 8500);
        CHECK(stmt.entries[3].balance_after_cents == 13500);
    }

    // --- Monthly statements: a month with no transactions ---
    {
        Account acc("S2", "Quiet User", 0);
        CHECK(acc.deposit(20000, "pay", "deposit", "2026-02-10"));
        CHECK(acc.withdraw(3000, "bills", "withdraw", "2026-02-15"));
        const auto march = acc.monthly_statement(2026, 3);
        CHECK(march.entries.empty());
        CHECK(march.opening_balance_cents == 17000);  // February's net carries over
        CHECK(march.total_deposits_cents == 0);
        CHECK(march.total_withdrawals_cents == 0);
        CHECK(march.closing_balance_cents == 17000);
        const auto feb = acc.monthly_statement(2026, 2);
        CHECK(feb.opening_balance_cents == 0);
        CHECK(feb.closing_balance_cents == 17000);
        CHECK(feb.entries.size() == 2);
        CHECK(feb.entries[1].balance_after_cents == 17000);
    }

    // --- Monthly statements: opening balance from a prior month ---
    {
        Bank bank;
        CHECK(bank.create_account("S3", "Early Bird", 5000, "2026-01-15"));
        const auto feb = bank.find_account("S3")->monthly_statement(2026, 2);
        CHECK(feb.entries.empty());
        CHECK(feb.opening_balance_cents == 5000);
        CHECK(feb.closing_balance_cents == 5000);
    }

    // --- Monthly statements: transfers count as in/out ---
    {
        Bank bank;
        CHECK(bank.create_account("T1", "Ann", 0));
        CHECK(bank.create_account("T2", "Ben", 0));
        CHECK(bank.find_account("T1")->deposit(10000, "pay", "deposit", "2026-04-01"));
        CHECK(bank.transfer("T1", "T2", 4000, "2026-04-05"));
        const auto t1 = bank.find_account("T1")->monthly_statement(2026, 4);
        CHECK(t1.opening_balance_cents == 0);
        CHECK(t1.total_deposits_cents == 10000);
        CHECK(t1.total_withdrawals_cents == 4000);  // transfer-out
        CHECK(t1.closing_balance_cents == 6000);
        CHECK(t1.entries.size() == 2);
        CHECK(t1.entries[1].type == "transfer-out");
        CHECK(t1.entries[1].balance_after_cents == 6000);
        const auto t2 = bank.find_account("T2")->monthly_statement(2026, 4);
        CHECK(t2.total_deposits_cents == 4000);  // transfer-in
        CHECK(t2.total_withdrawals_cents == 0);
        CHECK(t2.closing_balance_cents == 4000);
        CHECK(t2.entries.size() == 1);
    }

    // --- Dates: a transaction with no date is stamped today ---
    {
        Account acc("S4", "Today User", 0);
        CHECK(acc.deposit(1000, "payday"));
        CHECK(acc.history().back().date == bank::current_date());
        const auto bad = acc.monthly_statement(2026, 13);  // invalid month
        CHECK(bad.entries.empty());
        CHECK(bad.closing_balance_cents == 0);
    }

    std::cout << checks_run << " checks, " << checks_failed << " failed.\n";
    if (checks_failed == 0) std::cout << "All tests passed.\n";
    return checks_failed == 0 ? 0 : 1;
}
