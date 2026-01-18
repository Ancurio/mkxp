# Test script for out of bounds non-int writes inaccuracy.
# Run via the "customScript" field in mkxp.json.

# Isolate IO for testing in RPG Maker
def test_print(s)
	$stdout.print s
end

def test_puts(s)
	puts s
end

def print_line
	puts "################"
end

print_line

t = Table.new(4,4,4)

test_print "Testing inbounds int writes.. "
begin
	t[2,2,2] = 14
	value = t[2,2,2]
	if value == 14
		test_puts "Passed!"
	else
		test_puts "Failed! Expected 14, got #{value}"
	end
rescue => e
	puts "Failed! Unexpected exception: #{e}"
end

test_print "Testing inbounds non-int writes.. "
begin
	t[2,2,2] = Object.new
	puts "Failed! No exception thrown"
rescue => e
	if e.is_a?(TypeError)
		test_puts "Passed!"
	else
		test_puts "Failed! Wrong exception type #{e.class}"
	end
end

test_print "Testing oob int writes.. "
begin
	t[8,8,8] = 16
	value = t[8,8,8]
	if value.nil?
		test_puts "Passed!"
	else
		test_puts "Failed! Unexpected value #{value.inspect}"
	end
rescue => e
	test_puts "Failed! Unexpected exception: #{e}"
end

test_print "Testing oob non-int writes.. "
begin
	t[8,8,8] = Object.new
	test_puts "Passed!"
rescue => e
	test_puts "Failed! Unexpected exception: #{e}"
end

print_line
