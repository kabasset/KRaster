// @copyright 2022-2024, Antoine Basset (CNES)
// This file is part of Linx <github.com/kabasset/Linx>
// SPDX-License-Identifier: Apache-2.0

#ifndef _LINXRUN_PROGRAMOPTIONS_H
#define _LINXRUN_PROGRAMOPTIONS_H

#include "Linx/Base/TypeUtils.h" // LINX_FORWARD

#include <boost/program_options.hpp>
#include <iostream>
#include <sstream>

namespace Linx {

/**
 * @brief Helper class to declare positional, named and flag options, as well as some help message.
 * 
 * Here is an example use case for the following command line:
 * \verbatim Program <positional> --named1 <value1> -f --named2 <value2> \endverbatim
 * 
 * Here's an example program to handle it:
 * \code
 * std::pair<OptionsDescription, PositionalOptionsDescription> defineProgramArguments() override
 * {
 *   ProgramOptions options("My program");
 *   options.positional<std::string>("positional", "Positional option");
 *   options.named<int>("named1", "Named option 1");
 *   options.named<int>("named2", "Named option 2");
 *   options.flag("flag,f", "Flag");
 *   return options.as_pair();
 * }
 * \endcode
 */
class ProgramOptions {
public:

  /**
   * @brief Shortcut for Boost.ProgramOptions type.
   */
  using OptionsDescription = boost::program_options::options_description;

  /**
   * @brief Shortcut for Boost.ProgramOptions type.
   */
  using PositionalOptionsDescription = boost::program_options::positional_options_description;

  /**
   * @brief Shortcut for Boost.ProgramOptions type.
   */
  using ValueSemantics = boost::program_options::value_semantic;

  /**
   * @brief Helper class to print help messages!
   */
  class Help {
  public:

    /**
     * @brief Constructor.
     */
    explicit Help(const std::string& description) :
        m_desc(description), m_usage(" [options]"), m_positionals(), m_nameds()
    {}

    /**
     * @brief Check whether an option has a short name.
     */
    static bool has_short_name(const std::string& name)
    {
      return name.length() > 3 && name[name.length() - 2] == ',';
    }

    /**
     * @brief Get the long name of an option.
     */
    static std::string long_name(const std::string& name)
    {
      if (has_short_name(name)) {
        return name.substr(0, name.length() - 2);
      }
      return name;
    }

    /**
     * @brief Declare a positional option.
     */
    void positional(const std::string& name, const std::string& description)
    {
      const auto argument = "<" + long_name(name) + ">";
      m_usage += " " + argument;
      m_positionals.emplace_back(argument + "\n      " + append_dot(description));
    }

    /**
     * @brief Declare a positional option with default value.
     */
    template <typename T>
    void positional(const std::string& name, const std::string& description, T&& default_value)
    {
      const auto argument = "<" + long_name(name) + ">";
      m_usage += " [" + argument + "]";
      m_positionals.emplace_back(argument + "\n      " + append_dot(description));
      with_default(m_positionals.back(), LINX_FORWARD(default_value));
    }

    /**
     * @brief Declare a named option.
     */
    void named(const std::string& name, const std::string& description)
    {
      auto option = has_short_name(name) ? std::string {'-', name.back(), ',', ' '} : std::string();
      const auto ln = long_name(name);
      option += "--" + ln + " <" + ln + ">\n      " + append_dot(description);
      m_nameds.push_back(std::move(option));
    }

    /**
     * @brief Declare a named option with default value.
     */
    template <typename T>
    void named(const std::string& name, const std::string& description, T&& default_value)
    {
      named(name, description);
      with_default(m_nameds.back(), LINX_FORWARD(default_value));
    }

    void flag(const std::string& name, const std::string& description)
    {
      auto option = has_short_name(name) ? std::string {'-', name.back(), ',', ' '} : std::string();
      const auto ln = long_name(name);
      option += "--" + ln + "\n      " + append_dot(description);
      m_nameds.push_back(std::move(option));
    }

    /**
     * @brief Print the help message to a given stream.
     */
    void to_stream(const std::string& argv0, std::ostream& out = std::cout)
    {
      // Help
      if (not m_desc.empty()) {
        out << "\n" << m_desc << "\n";
      }

      // Usage
      out << "\nUsage:\n\n  " << argv0 << m_usage << "\n";
      // FIXME split program name?

      // Positional options
      for (const auto& o : m_positionals) {
        out << "\n  " << o;
      }
      if (not m_positionals.empty()) {
        out << "\n";
      }

      // Named options
      if (m_nameds.empty()) {
        return;
      }
      out << "\nOptions:\n";
      for (const auto& o : m_nameds) {
        out << "\n  " << o;
      }

      out << "\n\n";
      std::flush(out);
    }

  private:

    /**
     * @brief Add a default value to the last declared option.
     */
    template <typename T>
    void with_default(std::string& option, T&& value)
    {
      if constexpr (std::is_same_v<std::decay_t<T>, char>) {
        option.append("\n      [default: " + std::string {value} + "]");
      } else if constexpr (std::is_same_v<std::decay_t<T>, std::string>) {
        option.append("\n      [default: " + LINX_FORWARD(value) + "]");
      } else {
        option.append("\n      [default: " + std::to_string(LINX_FORWARD(value)) + "]");
      }
    }

    static std::string append_dot(const std::string description)
    {
      if (description.back() == '.') {
        return description;
      }
      return description + '.';
    }

  private:

    std::string m_desc; ///< The program description
    std::string m_usage; ///< The program usage
    std::vector<std::string> m_positionals; ///< The positional options description
    std::vector<std::string> m_nameds; ///< The named options description
  };

  /**
   * @brief Make a `ProgramOptions` with optional description string and help option.
   * @param description The program description
   * @param help The help option (disapled if parameter is empty)
   */
  ProgramOptions(const std::string& description = "", const std::string& help = "help,h") :
      m_named("Options", 120), m_add(m_named.add_options()), m_positional(), m_variables(), m_desc(description),
      m_help(help)
  {
    if (m_help.length() > 0) {
      flag(m_help.c_str(), "Print help message");
      m_help = Help::long_name(m_help);
    }
  }

  /**
   * @brief Declare a positional option.
   */
  template <typename T>
  void positional(const char* name, const char* description)
  {
    positional(name, boost::program_options::value<T>(), description);
    m_desc.positional(name, description);
  }

  /**
   * @brief Declare a positional option with default value.
   */
  template <typename T>
  void positional(const char* name, const char* description, T default_value)
  {
    positional(name, boost::program_options::value<T>()->default_value(default_value), description);
    m_desc.positional(name, description, default_value);
  }

  /**
   * @brief Declare a named option.
   * 
   * A short form (1-character) of the option can be provided, separated by a comma.
   */
  template <typename T>
  void named(const char* name, const char* description)
  {
    named(name, boost::program_options::value<T>(), description);
    m_desc.named(name, description);
  }

  /**
   * @brief Declare a named option with default value.
   * 
   * A short form (1-character) of the option can be provided, separated by a comma.
   */
  template <typename T>
  void named(const char* name, const char* description, T default_value)
  {
    named(name, boost::program_options::value<T>()->default_value(default_value), description);
    m_desc.named(name, description, default_value);
  }

  /**
   * @brief Declare a flag option.
   * 
   * A short form (1-character) of the option can be provided, separated by a comma.
   */
  void flag(const char* name, const char* description)
  {
    named(name, boost::program_options::value<bool>()->default_value(false)->implicit_value(true), description);
    m_desc.flag(name, description);
  }

  /**
   * @brief Get the named (flags included) and positional options as a pair.
   */
  [[deprecated]] std::pair<OptionsDescription, PositionalOptionsDescription> as_pair() const
  {
    return std::make_pair(m_named, m_positional);
  }

  /**
   * @brief Parse a command line.
   * 
   * If the help option was enabled and is in the command line, then the help message is printed and the program stops.
   */
  void parse(int argc, const char* const argv[])
  {
    boost::program_options::store(
        boost::program_options::command_line_parser(argc, argv).options(m_named).positional(m_positional).run(),
        m_variables);
    boost::program_options::notify(m_variables);
    if (not m_help.empty() && as<bool>(m_help.c_str())) {
      m_desc.to_stream(argv[0]);
      exit(0);
    }
  }

  /**
   * @brief Parse a command line (vector of `const char*` arguments).
   */
  void parse(const std::vector<const char*>& args)
  {
    parse(args.size(), args.data());
  }

  /**
   * @brief Parse a command line (vector of string arguments).
   */
  void parse(const std::vector<std::string>& args)
  {
    std::vector<const char*> cstr(args.size());
    std::transform(args.begin(), args.end(), cstr.begin(), [](const auto& a) {
      return a.c_str();
    });
    parse(cstr);
  }

  /**
   * @brief Parse a command line (space-separated arguments as a single string).
   */
  void parse(const std::string& args)
  {
    std::istringstream iss(args);
    parse(std::vector<std::string>(std::istream_iterator<std::string> {iss}, std::istream_iterator<std::string>()));
  }

  /**
   * @brief Check whether a given option is set.
   */
  bool has(const char* name) const
  {
    return m_variables.count(name); // FIXME incompatible with flags
  }

  /**
   * @brief Get the value of a given option.
   * 
   * Throws if the option is not set.
   */
  template <typename T>
  T as(const char* name) const
  {
    return m_variables[name].as<T>();
  }

private:

  /**
   * @brief Declare a positional option with custom semantics.
   */
  void positional(const char* name, const ValueSemantics* value, const char* description)
  {
    m_add(name, value, description);
    const int max_args = value->max_tokens();
    m_positional.add(name, max_args);
  }

  /**
   * @brief Declare a named option with custom semantics.
   * 
   * A short form (1-character) of the option can be provided, separated by a comma.
   */
  void named(const char* name, const ValueSemantics* value, const char* description)
  {
    m_add(name, value, description);
  }

private:

  OptionsDescription m_named;
  boost::program_options::options_description_easy_init m_add;
  PositionalOptionsDescription m_positional;
  boost::program_options::variables_map m_variables;
  Help m_desc;
  std::string m_help;
};

} // namespace Linx

#endif
