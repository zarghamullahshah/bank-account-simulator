#include "bank.h"

#include <algorithm>
#include <cmath>
#include <ctime>
#include <sstream>

namespace bank {

namespace {

// "deposit" and "transfer-in" move money in; everything else moves it out.
bool is_inflow(const std::string& type) {
    return type != "withdraw" && type != "transfer-out";
}

std::string month_prefix(int year, int month) {
    std::ostringstream out;
    out << year << '-';
    if (month < 10) out << '0';
    out << month;
    return out.str();
}

}  // namespace

std::string current_date() {
    const std::time_t now = std::time(nullptr);
    std::tm local{};
    if (localtime_r(&now, &local) == nullptr) return "1970-01-01";
    char buf[11];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d", &local);
    return buf;
}

Account::Account(std::string id, std::string owner, Cents opening_balance_cents,
                 const std::string& date)
    : id_(std::move(id)),
      owner_(std::move(owner)),
      balance_cents_(opening_balance_cents < 0 ? 0 : opening_balance_cents) {
    if (opening_balance_cents > 0) {
        history_.push_back({"deposit", opening_balance_cents, balance_cents_,
                            "opening balance",
                            date.empty() ? current_date() : date});
    }
}

bool Account::deposit(Cents amount_cents, const std::string& note,
                      const std::string& type, const std::string& date) {
    if (amount_cents <= 0) return false;
    balance_cents_ += amount_cents;
    history_.push_back({type, amount_cents, balance_cents_, note,
                        date.empty() ? current_date() : date});
    return true;
}

bool Account::withdraw(Cents amount_cents, const std::string& note,
                       const std::string& type, const std::string& date) {
    if (amount_cents <= 0 || amount_cents > balance_cents_) return false;
    balance_cents_ -= amount_cents;
    history_.push_back({type, amount_cents, balance_cents_, note,
                        date.empty() ? current_date() : date});
    return true;
}

bool Account::apply_monthly_interest(double monthly_rate_percent,
                                     const std::string& date) {
    if (monthly_rate_percent <= 0 || balance_cents_ <= 0) return false;
    const Cents interest = static_cast<Cents>(std::llround(
        static_cast<double>(balance_cents_) * monthly_rate_percent / 100.0));
    if (interest <= 0) return false;
    balance_cents_ += interest;
    history_.push_back({"interest", interest, balance_cents_,
                        "monthly interest",
                        date.empty() ? current_date() : date});
    return true;
}

MonthlyStatement Account::monthly_statement(int year, int month) const {
    MonthlyStatement stmt{year, month, 0, 0, 0, 0, {}};
    if (month < 1 || month > 12) return stmt;
    const std::string prefix = month_prefix(year, month);

    // Sort a copy by date so entries come out in date order even when
    // transactions were recorded out of order.
    std::vector<Transaction> ordered = history_;
    std::stable_sort(ordered.begin(), ordered.end(),
                     [](const Transaction& a, const Transaction& b) {
                         return a.date < b.date;
                     });

    // Opening balance = net of everything dated before the month. Running
    // balances are recomputed from it, not copied from the stored
    // balance-after values, so they stay correct for out-of-order entries.
    Cents opening = 0;
    for (const auto& tx : ordered) {
        if (tx.date.compare(0, prefix.size(), prefix) == 0) continue;
        if (tx.date < prefix) {
            opening += is_inflow(tx.type) ? tx.amount_cents : -tx.amount_cents;
        }
    }
    Cents running = opening;
    for (const auto& tx : ordered) {
        if (tx.date.compare(0, prefix.size(), prefix) != 0) continue;
        const Cents signed_amount =
            is_inflow(tx.type) ? tx.amount_cents : -tx.amount_cents;
        running += signed_amount;
        if (is_inflow(tx.type)) {
            stmt.total_deposits_cents += tx.amount_cents;
        } else {
            stmt.total_withdrawals_cents += tx.amount_cents;
        }
        stmt.entries.push_back(
            {tx.date, tx.type, signed_amount, running});
    }
    stmt.opening_balance_cents = opening;
    stmt.closing_balance_cents = running;
    return stmt;
}

std::string Account::format(Cents amount_cents) {
    std::ostringstream out;
    if (amount_cents < 0) out << '-';
    const Cents abs_amount = std::abs(amount_cents);
    out << '$' << (abs_amount / 100) << '.';
    const Cents cents_part = abs_amount % 100;
    if (cents_part < 10) out << '0';
    out << cents_part;
    return out.str();
}

bool Bank::create_account(const std::string& id, const std::string& owner,
                          Cents opening_balance_cents,
                          const std::string& date) {
    if (find_account(id) != nullptr) return false;
    accounts_.emplace_back(id, owner, opening_balance_cents, date);
    return true;
}

Account* Bank::find_account(const std::string& id) {
    for (auto& account : accounts_) {
        if (account.id() == id) return &account;
    }
    return nullptr;
}

const Account* Bank::find_account(const std::string& id) const {
    for (const auto& account : accounts_) {
        if (account.id() == id) return &account;
    }
    return nullptr;
}

bool Bank::transfer(const std::string& from_id, const std::string& to_id,
                    Cents amount_cents, const std::string& date) {
    if (amount_cents <= 0 || from_id == to_id) return false;
    Account* from = find_account(from_id);
    Account* to = find_account(to_id);
    if (from == nullptr || to == nullptr) return false;
    if (amount_cents > from->balance_cents()) return false;
    // Withdraw first; it cannot fail after the checks above, so the
    // transfer is atomic.
    from->withdraw(amount_cents, "transfer to " + to_id, "transfer-out", date);
    to->deposit(amount_cents, "transfer from " + from_id, "transfer-in", date);
    return true;
}

}  // namespace bank
