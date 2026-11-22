// Demo: runs a small scripted banking scenario and prints statements.
#include <iostream>

#include "bank.h"

using bank::Account;
using bank::Bank;
using bank::Cents;

static void print_statement(const Account& account) {
    std::cout << "Account " << account.id() << " (" << account.owner() << ")\n";
    std::cout << "  Balance: " << Account::format(account.balance_cents()) << "\n";
    for (const auto& tx : account.history()) {
        std::cout << "  " << tx.type << " " << Account::format(tx.amount_cents)
                  << " -> " << Account::format(tx.balance_after_cents);
        if (!tx.note.empty()) std::cout << "  (" << tx.note << ")";
        std::cout << "\n";
    }
}

int main() {
    Bank bank;
    bank.create_account("ACC-1001", "Ayesha Khan", 50000);   // $500.00
    bank.create_account("ACC-1002", "Bilal Ahmed", 25000);   // $250.00

    bank.find_account("ACC-1001")->deposit(12550, "salary");        // +$125.50
    bank.find_account("ACC-1001")->withdraw(4000, "groceries");     // -$40.00
    bank.transfer("ACC-1001", "ACC-1002", 10000);                   // $100.00

    if (!bank.find_account("ACC-1002")->withdraw(9999999, "too much")) {
        std::cout << "Overdraft correctly refused.\n\n";
    }

    print_statement(*bank.find_account("ACC-1001"));
    std::cout << "\n";
    print_statement(*bank.find_account("ACC-1002"));
    return 0;
}
