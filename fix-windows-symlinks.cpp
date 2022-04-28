#include <filesystem>
#include <ranges>
#include <algorithm>
#include <memory>
#include <iostream>
#include <cassert>

#include <fmt/core.h>
#include <fmt/ostream.h>
#include <boost/process.hpp>

namespace fs = std::filesystem;
namespace bproc = boost::process;

int main (int argc, char* argv[])
{
    const auto directory_to_scan = argc > 1 ?  fs::path{ argv[1] } : fs::current_path();

    auto dir_symlinks = fs::recursive_directory_iterator(directory_to_scan)
        | std::views::filter( [](const auto& entry){ return fs::is_symlink(entry.path()); })
        | std::views::transform([](const auto& entry){
            const auto target = fs::absolute(entry.path().parent_path() / fs::read_symlink(entry.path()));
            return std::make_pair(entry.path(), target);
        })
        | std::views::filter( [](const auto& entry){ 
            return fs::is_directory(entry.second);
        });
        
    for( const auto& [symlink, target] : dir_symlinks )
    { 
        fmt::print("symlink to recreate: {} -> {} ({}) \n", symlink, fs::read_symlink(symlink), target);
        assert(fs::is_directory(target));
    }

    fmt::print("\nConfirm symlink rebuilds by typing 'yes'? ");
    std::string input;
    std::cin >> input;
    if(input != "yes")
    {
        fmt::print("\nSymlink rebuild cancelled.\n");
        return EXIT_SUCCESS;
    }

    fmt::print("\nRebuilding symlinks...\n");

    struct ScopeExit{
        const fs::path initial_working_path = fs::current_path();
        ~ScopeExit(){
            fs::current_path(initial_working_path);
        }
    } reset_working_path_before_exiting;

    for( const auto& [symlink, target] : dir_symlinks )
    { 
        assert(fs::is_directory(target));
        const auto relative_target = fs::read_symlink(symlink);

        fs::remove(symlink);
        assert(!fs::is_symlink(symlink));

        fs::current_path(symlink.parent_path());
        fs::create_directory_symlink(relative_target, symlink.filename());
        assert(fs::is_symlink(symlink));
    }

    fmt::print("Rebuilding symlinks - Done\n");

}
