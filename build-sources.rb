#!/usr/bin/env ruby

OS_DIR = /(?i:\/[a-z0-9 \-_\.@]+\.([^\/\\]+))\//
SOURCE_FILE = /(?i:\.(c(c|pp|xx|\+\+)?|m{1,2}))$/
CPP_FILE = /(?i:\.(c(c|pp|xx|\+\+)|mm))$/
INCLUDE_FILE = /^\s*#\s*include\s+(["<])([^">]+)[">]/
EXTENSION = /^\.(.*)$/

# filepath = path to the source file (relative or absolute)
# search_path = paths to search for system includes (relative or absolute)
def pull_dependencies(filepath, search_paths=[], depth=1, checked=[])
  if depth > 32
    raise "We've gone too deep, Captain!"
  end

  abs_path = File.absolute_path(filepath)
  return [] if checked.include?(abs_path) && depth != 0
  checked << abs_path

  dir = File.dirname(filepath)
  deps = []

  raise "File does not exist" unless File.exists?(filepath) && ! File.directory?(filepath)

  File.open(filepath, "r") {
    |ios|
    ios.each_line {
      |line|
      if (line =~ INCLUDE_FILE)
        scope = $1
        incl_path = $2
        if scope == '<' # search
          found = false

          search_paths.each {
            |search_path|
            check_path = "#{search_path}/#{incl_path}"
            # puts "Checking for <#{check_path}>"
            if File.exists?(check_path) && ! File.directory?(check_path)
              deps << check_path
              deps |= pull_dependencies(check_path, search_paths, depth + 1, checked)
              found = true
              break
            end
          }

          if ! found
            # puts "Unable to locate <#{incl_path}>"
          end

        elsif scope === '"' # relative
          check_path = "#{dir}/#{incl_path}"
          # puts "Checking for \"#{check_path}\""
          if File.exists?(check_path) && ! File.directory?(check_path)
            deps << check_path
            deps |= pull_dependencies(check_path, search_paths, depth + 1, checked)
          else
            # puts "Unable to locate \"#{incl_path}\""
          end
        end
      end

    }
  }

  return deps
end

def get_sources(search_in=".", for_os=[])
  for_os ||= []
  for_os = for_os.map { |e| e.downcase }
  search_in = search_in.chomp '/'

  skips = []
  paths = Dir.glob("#{search_in}/**/*").select {
    # ignore non-C/C++ source, files starting/in a dir starting with a ., and files in dirs marked .exclude
    |e|
    if File.basename(e).downcase == "exclude"
      d = File.dirname(e)
      skips << d
      $stderr.puts "Skipping <#{d}>"
    end

    matches = File.extname(e) =~ SOURCE_FILE && ! (e =~ /\/\.[^\.]/) && ! (e.include? '.exclude/')
    if matches
      if e =~ OS_DIR
        for_os.include? $1.downcase
      else
        true
      end
    end
  }.select {
    |e|
    skip = false
    skips.each { |d| skip ||= e.start_with? d }

    ! skip
  }
end

def dump_make(ios, sources)
  objects = sources.map {
    |e|
    n = "#{e.chomp(File.extname(e))}.o"
    raise "Failed to convert source file to object file" if e == n
    n
  }

  ios.puts "SOURCES:=\\"
  ios.puts "  " + sources.join(" \\\n  ")
  ios.write $/

  ios.puts "OBJECTS:=\\"
  ios.puts "  " + objects.join(" \\\n  ")
  ios.write $/

  sources.each {
    |filepath|

    filedeps = pull_dependencies(filepath, ["src"])

    ios.puts "#{filepath.sub(SOURCE_FILE, '.o')}: #{filepath} #{filedeps.join ' '}"
    if File.extname(filepath) =~ CPP_FILE
      ios.puts "\t$(CXX) $(CFLAGS) $(CXXFLAGS) -c $< -o $@\n\n"
    else
      ios.puts "\t$(CC) $(CFLAGS) -c $< -o $@\n\n"
    end
  }
end

SHORT_OPT = /^-(?<name>[a-zA-Z0-9])(?<value>.*)$/
LONG_OPT = /^--(?<name>\w+)(?:(\s*=\s*|\s+)(?<value>.*))?$/

# opts = {:someArgument => {:kind => :flag, :alias => :real}} specifies an option is a switch instead of a
# variable option
def parse_args(args, opts={})
  args = args.clone

  inputs = []
  flags = {}

  until args.empty?
    arg = args.shift

    name = nil

    if ! (match = (arg.match(SHORT_OPT) || arg.match(LONG_OPT))).nil?
      name = match[:name].to_sym
      opt = opts[name]
      is_flag = nil
      allow_multiple = nil
      if opt.nil?
        is_flag = match[:value].empty?
      else
        alias_opt = opt[:alias]
        unless alias_opt.nil?
          name = alias_opt
          opt = opts[name]
        end

        unless opt.nil?
          is_flag = opt[:flag] unless opt[:flag].nil?
          allow_multiple = opt[:multiple] unless opt[:multiple].nil?
        end
      end

      value = is_flag ? true : match[:value]

      if is_flag != true && (value.nil? || value.empty?)
        value = args.shift
      end

      raise "Invalid argument: #{arg}" if (value.nil? || value.empty?) && ! is_flag

      if flags.include?(name) && allow_multiple != false
        cur_value = flags[name]
        if cur_value.is_a? Array
          value = [*cur_value, value]
        else
          value = [cur_value, value]
        end
      elsif allow_multiple != false
        value = [value]
      end

      flags.store name, value
    else
      inputs << arg
    end
  end

  return inputs, flags
end

inputs, flags = parse_args(ARGV, :t => {:alias => :target}, :target => {:multiple => true, :flag => false})
target = flags[:target] || ""
target = [target] unless target.is_a? Array
sources = get_sources('src', target)
dump_make $stdout, sources
