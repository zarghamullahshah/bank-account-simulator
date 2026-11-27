#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace bank {

// Money is stored as integer cents so balances never suffer float rounding.
using Cents = std::int64_t;

struct Transaction {
    std::string type;        // "deposit", "withdraw", "transfer-in", "transfer-out"
    Cents amount_cents;
    Cents balance_after_cents;
    std::string note;
    std::string date;        // ISO "YYYY-MM-DD" the transaction happened
};

// One line of a monthly statement: amount is signed, + for money in
// (deposit, transfer-in), - for money out (withdraw, transfer-out).
struct StatementEntry {
    std::string date;  // "YYYY-MM-DD"
    std::string type;
    Cents amount_cents;
    Cents balance_after_cents;  // running balance after this entry
};

struct MonthlyStatement {
    int year;
    int month;
    Cents opening_balance_cents;
    Cents total_deposits_cents;
    Cents total_withdrawals_cents;
    Cents closing_balance_cents;
    std::vector<StatementEntry> entries;  // in date order
};

// Today's local date as "YYYY-MM-DD".
std::string current_date();

class Account {
public:
    Account(std::string id, std::string owner, Cents opening_balance_cents = 0,
            const std::string& date = "");  // "" = today

    const std::string& id() const { return id_; }
    const std::string& owner() const { return owner_; }
    Cents balance_cents() const { return balance_cents_; }
    const std::vector<Transaction>& history() const { return history_; }

    // Returns false (and changes nothing) when amount <= 0.
    // `date` is the ISO date the transaction happened; "" means today.
    bool deposit(Cents amount_cents, const std::string& note = "",
                 const std::string& type = "deposit",
                 const std::string& date = "");
    // Returns false (and changes nothing) when amount <= 0 or exceeds balance.
    bool withdraw(Cents amount_cents, const std::string& note = "",
                  const std::string& type = "withdraw",
                  const std::string& date = "");

    // Credits monthly interest on a positive balance: the balance times
    // the monthly rate, rounded to the nearest cent, recorded as an
    // "interest" transaction. Returns false (and changes nothing) when the
    // rate is <= 0, the balance is <= 0, or the interest rounds to zero.
    // `date` is the ISO date the interest is credited; "" means today.
    bool apply_monthly_interest(double monthly_rate_percent,
                                const std::string& date = "");

    // Statement for one calendar month: every transaction in that month in
    // date order with a running balance, plus opening/closing balances and
    // totals. A month outside 1-12 returns an empty statement.
    MonthlyStatement monthly_statement(int year, int month) const;

    static std::string format(Cents amount_cents);  // e.g. -1234 -> "-$12.34"

private:
    std::string id_;
    std::string owner_;
    Cents balance_cents_;
    std::vector<Transaction> history_;
};

class Bank {
public:
    // Returns false when an account with this id already exists.
    // `date` is the ISO date of the opening-balance transaction; "" = today.
    bool create_account(const std::string& id, const std::string& owner,
                        Cents opening_balance_cents = 0,
                        const std::string& date = "");
    Account* find_account(const std::string& id);
    const Account* find_account(const std::string& id) const;

    // Atomic: either both legs happen or neither does. False when either
    // account is missing, amount <= 0, or the source lacks funds.
    // `date` is the ISO date of both legs; "" means today.
    bool transfer(const std::string& from_id, const std::string& to_id,
                  Cents amount_cents, const std::string& date = "");

    std::size_t account_count() const { return accounts_.size(); }

private:
    std::vector<Account> accounts_;
};

}  // namespace bank
