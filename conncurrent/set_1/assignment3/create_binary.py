import random
import struct

numbers = list(range(1000))

#Add duplicates
num_duplicates = 20
duplicates = random.sample(numbers, num_duplicates)
numbers.extend(duplicates)

#Shuffle integers
random.shuffle(numbers)

# File path for the binary file
file_path = "shuffled_numbers.bin"

with open(file_path, "wb") as binary_file:
    for number in numbers:
        binary_file.write(struct.pack('i', number))


#WITHOUT DUPLICATES
# import random
# import struct

# numbers = list(range(1000))
# random.shuffle(numbers)

# file_path = "shuffled_numbers.bin"

# with open(file_path, "wb") as binary_file:
#     for number in numbers:
#         binary_file.write(struct.pack('i', number))
