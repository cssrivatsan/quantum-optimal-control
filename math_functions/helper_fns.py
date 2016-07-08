import numpy as np

def qft(N):

    phase = 2.0j * np.pi / (2**N)
    L, M = np.meshgrid(np.arange(2**N), np.arange(2**N))
    L = np.exp(phase * (L * M))
    q = 1.0 / np.sqrt(2**N) * L
    return q
    
def hamming_distance(x):
    tot = 0
    while x:
        tot += 1
        x &= x - 1
    return tot

def Hadamard (N=1):
    
    Had = (2.0 ** (-N / 2.0)) * np.array([[((-1) ** hamming_distance(i & j))
                                      for i in range(2 ** N)]
                                     for j in range(2 ** N)])
    return Had

def concerned(N,levels):
    concern = []
    for ii in range (levels**N):
        ii_b = Basis(ii,N,levels)
        if is_binary(ii_b):
            concern.append(ii)
    return concern
        
def is_binary(num):
    flag = True
    for c in num: 
        if c!='0' and c!='1':
            flag = False
            break
    return flag
    
def transmon_gate(gate,levels):
    N = int(np.log2(len(gate)))
    result = np.identity(levels**N,dtype=complex)
    for ii in range (len(result)):
        for jj in range(len(result)):
            ii_b = Basis(ii,N,levels)
            jj_b = Basis(jj,N,levels)
            if is_binary(ii_b) and is_binary(jj_b):
                result[ii,jj]=gate[int(ii_b, 2),int(jj_b, 2)]
                
    return result

def Bin(a,N):
    a_bin = np.binary_repr(a)
    while len(a_bin) < N:
        a_bin = '0'+a_bin
    return a_bin

def baseN(num,b,numerals="0123456789abcdefghijklmnopqrstuvwxyz"):
    return ((num == 0) and numerals[0]) or (baseN(num // b, b, numerals).lstrip(numerals[0]) + numerals[num % b])

def Basis(a,N,r):
    a_new = baseN(a,r)
    while len(a_new) < N:
        a_new = '0'+a_new
    return a_new
    
    
def kron_all(op,num,op_2): # returns an addition of sth like xii + ixi + iix for op =x and op_2 =i
    total = np.zeros([len(op)**num,len(op)**num])
    a=op
    for jj in range(num):
        if jj != 0:
            a = op_2
        else:
            a = op
            
        for ii in range(num-1):
            if (jj - ii) == 1:
                
                b = op
            else:
                b = op_2
            a = np.kron(a,b)
        total = total + a
    return a    

def multi_kron(op,num): #returns xx...x
    a=op
    for ii in range(num-1):
        a = np.kron(a,op)
    return a

def append_separate_krons(op,name,num,state_num,Hops,Hnames,ops_max_amp,amp=4.0): #appends xii,ixi,iix separately
    string = name
    I_q = np.identity(state_num)
    x = 1
    y = 1
    z = 1
    X1 = op
    while(x < num):
        X1 = np.kron(X1, I_q)
        x = x + 1
    Hops.append(X1)
    ops_max_amp.append(amp)
    x = 1
    while(x < num):
        string = string + 'i'
        x = x+1
    Hnames.append(string)

    x = 1

    while(x < num):
        X1 = I_q
        string = 'i'
        while(y<num):
            if(y==x):
                X1 = np.kron(X1, op)
                y = y + 1
                string = string + name
            else:
                X1 = np.kron(X1, I_q)
                y = y + 1
                string = string + 'i'
        x = x + 1
        y=1
        Hops.append(X1)
        ops_max_amp.append(amp)
        Hnames.append(string)
    return Hops,Hnames,ops_max_amp