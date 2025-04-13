def word_to_byte(addr, index , lowerfirst):
    value = (addr >> ( 8 * ( index -1)) & 0x000000FF )
    return value


data_buf = [0] * 6

go_address  = input("\n   Please enter 4 bytes go address in hex:")
go_address = int(go_address, 16)
data_buf[0] = 10-1 
data_buf[1] = 85 
data_buf[2] = word_to_byte(go_address,1,1) 
data_buf[3] = word_to_byte(go_address,2,1) 
data_buf[4] = word_to_byte(go_address,3,1) 
data_buf[5] = word_to_byte(go_address,4,1)



print( go_address )