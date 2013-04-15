#!/usr/bin/env ruby -w

FORMAT_REGEX = %r{ ^
  (?<key> [^\s] +?) \s*
  (?: = \s* (?<value> [^\s]+ ) \s* )?
  $ }x

def extract_format(options)
  options.inject({}) { |formats, opt|
    key   = ""
    value = ""

    if ! (match = opt.match(FORMAT_REGEX)).nil?
      key   = match["key"]
      value = match["value"] unless match["value"].nil?

      if formats.include? key
        formats[key] = "#{formats[key]} #{value}"
      else
        formats[key] = value
      end

      formats
    end
  }
end

def format_source(source, formats)
  result = Marshal.load(Marshal.dump(source))
  formats.each { |find, replace|
    result.gsub!("${#{find}}", replace)
  }
  result.gsub!(/\$\{[^\}]+?\}/, "") # strip any unmatches formats
  return result
end

raise "No input argument to format.rb"  if ARGV.length == 0
raise "No output argument to format.rb" if ARGV.length == 1

input_path  = ARGV[0]
output_path = ARGV[1]
formats     = extract_format(ARGV.length > 2 ? ARGV[2..-1] : [])

source = if input_path == "-"
  STDIN.read
else
  File.open(input_path, "r") { |io| io.read }
end

output = format_source(source, formats)
if output_path == "-"
  STDOUT.write output
else
  File.open(output_path, "w") { |io| io.write output }
end
