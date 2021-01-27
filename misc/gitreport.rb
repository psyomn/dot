#!/usr/bin/env ruby

require 'open3'
require 'date' # to_datetime
require 'csv'
require 'erb'
require 'pry'

# TODO: parametrize this
AuthorAliases = ["psyomn", "Simon Symeonidis"]

if ARGV.count == 0
  puts " will scan your projects stored under git, and create a report"
  puts " of all your commits"
  puts "usage: "
  puts "  gitreport.rb <codedump>"
  exit 1
end

repos = []
Dir.glob(ARGV[0] + "**/*", File::FNM_DOTMATCH).each do |dir|
  repos.push(dir) if dir.match(/\.git$/)
end

puts "found #{repos.count} repo(s)"
puts "sample 10:"
repos.shuffle.take(10).map { |el| puts "- #{el}" }

puts "finding your contributions..."

class CommitEntry
  def initialize(repo_name, commit, date, subject)
    @repo_name = repo_name.split('/')[-2]
    @repo_path = repo_name
    @commit = commit
    @date = Time.at(date.to_i).to_datetime
    @subject = subject
  end
  attr_accessor :repo_name, :repo_path, :commit, :date, :subject
end

entries = []
command = "git log --format=\"%H %at %s\" --author=\"#{AuthorAliases.join('\|')}\""
repos.each do |repo|
  stdout_str, stderr_str, code = Open3.capture3("cd #{repo} && " + command)

  stdout_str.lines.each do |line|
    parts = line.strip.split(/\s/)
    commit = parts[0]
    date = parts[1]
    subject = parts[2..-1]&.join(' ')
    entries.push(CommitEntry.new(repo, commit, date, subject))
  end
end

entries.sort! { |x,y| x.date <=> y.date }

begin # html per year breakdowns
  Css = <<EOF
  table {
    border: 1px black solid;
  }

  td {
    border: 1px gray solid;
    text-align: left;
    vertical-align: top;
  }

  .repo-name {
    border: 1px gray solid;
    padding: 2px;
    margin: 5px;
    display: inline-block;
  }

  .commit {
    border: 1px gray solid;
    font-family: Courier, monospace;
  }
EOF

  YearlyERB = <<EOF
<html>
  <head>
    <style> <%= Css %> </style>
  </head>
  <body>
    <div class="content">
      <div><h1> <%= entries[0].date.year %> </h1></div>
      <div> <p> <b>commits</b>: <%= entries.count %> </p> </div>
      <div><b>repos contributed to</b>:
        <% entries.collect(&:repo_name).uniq.sort.each do |name| %>
          <span class="repo-name"> <%= name %>(<%= entries.count { |e| e.repo_name == name }%>) </span>
        <% end %>
      </div>

      <% group_by_month = entries.group_by { |el| el.date.month } %>

      <div class="per-month">
        <table>
        <% group_by_month.each do |month, entries| %>
          <tr>
            <td>
              <p> <%= Date::MONTHNAMES[month] %> </p>
              <p> projects: <%= entries.uniq { |e| e.repo_name }.count %> </p>
              <p> commits: <%= entries.count %> </p>
            </td>
            <td>
              <ul>
              <% entries.uniq{|el| el.commit}.group_by(&:repo_name).each do |name, entries| %>
                <li> <%= name %> (<%= entries.count %>):
                   <ul>
                     <% entries.each do |entry| %>
                       <li> <span class="commit"><%= entry.commit[0..8] %></span>: <%= entry.subject %></li>
                     <% end %>
                   </ul>
                </li>
              <% end %>
              </ul>
            </td>
          </tr>
        <% end %>
        </table>
      </div>
  </body>
</html>
EOF

  entries_by_year = entries.group_by { |el| el.date.year }

  entries_by_year.each do |year, entries|
    document = ERB.new(YearlyERB)
    html_text = document.result(binding)
    File.write(year.to_s + ".html", html_text)
  end
end
