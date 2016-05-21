$c = Hash.new

File.readlines(ARGV.shift).each {|line|
  line.chomp!
  raise if line =~ /^!/
  hashsum, fn = line.split("\t", 2)
  ($c[hashsum] ||= []) << fn
}

$c.each {|k,arr|
  next if arr.size == 1
  puts "#{k}"
  arr.each {|v| 
    puts " - #{v}" 
    `display "#{v}"` 
  }
}
