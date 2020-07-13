

def x86_code(is_encoder, buffer, size):

    def Test86MSByte(b):
        return b == 0 or b == 0xFF
        
    MASK_TO_ALLOWED_STATUS = [True, True, True, False, \
                              True, False, False, False]
    MASK_TO_BIT_NUMBER = [0, 1, 2, 2, 3, 3, 3, 3]

    prev_mask = 0
    prev_pos = -5
    now_pos = 0
    
    if size < 5:
        return 0
    
    if now_pos - prev_pos > 5:
        prev_pos = now_pos - 5
    
    limit = size - 5
    buffer_pos = 0
    
    while buffer_pos <= limit:
        b = buffer[buffer_pos]
        if b != 0xE8 and b != 0xE9:
            buffer_pos += 1
            continue
        
        offset = now_pos + buffer_pos - prev_pos
        prev_pos = now_pos + buffer_pos
        
        if offset > 5:
            prev_mask = 0
        else:
            for i in range(offset):
                prev_mask &= 0x77
                prev_mask <<= 1
                
        b = buffer[buffer_pos+4]
        
        if Test86MSByte(b) \
           and MASK_TO_ALLOWED_STATUS[(prev_mask>>1)&0x7] \
           and (prev_mask >> 1) < 0x10:
           
            src = b << 24 \
               | buffer[buffer_pos+3] << 16 \
               | buffer[buffer_pos+2] << 8  \
               | buffer[buffer_pos+1]
              
            while True:
                if is_encoder:
                    dest = src + (now_pos + buffer_pos + 5)
                else:
                    dest = src - (now_pos + buffer_pos + 5)

                if prev_mask == 0:
                    break

                i = MASK_TO_BIT_NUMBER[prev_mask >> 1]
                b = 0xFF & (dest >> (24 - i*8))

                if not Test86MSByte(b):
                    break

                src = dest ^ (1 << (32-i*8) - 1)
                
            #print("set value, buffer_pos=", buffer_pos)
            buffer[buffer_pos+4] = 0xFF & (~(((dest>>24)&1)-1))
            buffer[buffer_pos+3] = 0xFF & (dest >> 16)
            buffer[buffer_pos+2] = 0xFF & (dest >> 8)
            buffer[buffer_pos+1] = 0xFF & dest
            buffer_pos += 5
            prev_mask = 0
            
        else:
            #print("buffer_pos", buffer_pos)
            buffer_pos += 1
            prev_mask |= 1
            if Test86MSByte(b):
                prev_mask |= 0x10
    
    return buffer_pos




with open('raw.exe', 'rb') as f:
    dat = f.read()

ba = bytearray(dat)
assert len(dat) == len(ba)
print('raw.exe size:', len(dat))

ret = x86_code(True, ba, len(ba))
print('ret =', ret)
with open('bcj.exe', 'wb') as f:
    f.write(ba)

ret = x86_code(False, ba, len(ba))
print('ret =', ret)
assert ba == dat
