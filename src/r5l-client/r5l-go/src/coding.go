/*
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */

package main

import (
	"fmt"
	"net"
)

func EncodeZigZag(n uint64) uint64 {
	return (n << 1) ^ (n >> 63)
}

func DecodeZigZag(n uint64) uint64 {
	return (n >> 1) ^ -(n & 1)
}

func UIntBytes(value uint) uint {
	return UInt64Bytes(uint64(value))
}

func UInt64Bytes(value uint64) uint {
	for i := uint(1); i < 8; i++ {
		if value < (1 << (8 * i)) {
			return i
		}
	}
	return 8
}

func EncodeUInt(length uint, value uint64) []byte {
	buf := make([]byte, length)
	EncodeUInt64WithBuffer(buf, 0, length, value)
	return buf
}

func EncodeUIntWithBuffer(buf []byte, offset uint, length uint, value uint) {
	EncodeUInt64WithBuffer(buf, offset, length, uint64(value))
}

func EncodeUInt64WithBuffer(buf []byte, offset uint, length uint, value uint64) {
	for i := uint(0); i < length; i++ {
		buf[offset+i] = byte((value >> (8 * i)) & 0xff)
	}
}

func DecodeUInt(buf []byte, offset uint, length uint) uint {
	return uint(DecodeUInt64(buf, offset, length))
}

func DecodeUInt64(buf []byte, offset uint, length uint) uint64 {
	var result uint64 = 0
	for i := uint(0); i < length; i++ {
		result += uint64(buf[offset+i]) << (8 * i)
	}
	return result
}

func BuildMsgHead(pkgType uint, msgType uint, msgId uint64, fwdLen uint, bodyLen uint, dataLen uint) []byte {
	msgIdBytes := UInt64Bytes(msgId)
	msgTypeBytes := UIntBytes(msgType)
	fwdBytes := UIntBytes(fwdLen)
	bodyBytes := UIntBytes(bodyLen)
	dataBytes := UIntBytes(dataLen)

	head := make([]byte, 2+msgIdBytes+msgTypeBytes+fwdBytes+bodyBytes+dataBytes)
	head[0] = byte((pkgType << 4) | ((msgTypeBytes - 1) << 2) | fwdBytes)
	head[1] = byte(((msgIdBytes - 1) << 5) | (bodyBytes << 3) | dataBytes)

	offset := uint(2)
	EncodeUIntWithBuffer(head, offset, msgTypeBytes, msgType)
	offset += msgTypeBytes

	EncodeUInt64WithBuffer(head, offset, msgIdBytes, msgId)
	offset += msgIdBytes

	EncodeUIntWithBuffer(head, offset, fwdBytes, fwdLen)
	offset += fwdBytes

	EncodeUIntWithBuffer(head, offset, bodyBytes, bodyLen)
	offset += bodyBytes

	EncodeUIntWithBuffer(head, offset, dataBytes, dataLen)
	offset += dataBytes

	return head
}

func ParseMsgHead(buf []byte, pkgType *uint, msgType *uint, msgId *uint64, fwdLen *uint, bodyLen *uint, dataLen *uint) uint {
	//if len(buf) < 2: raise IncompleteIpcMsgError()
	*pkgType = (uint(buf[0]) >> 4) & 15
	msgTypeBytes := ((uint(buf[0]) >> 2) & 3) + 1
	fwdBytes := uint(buf[0]) & 3
	msgIdBytes := ((uint(buf[1]) >> 5) & 7) + 1
	bodyBytes := (uint(buf[1]) >> 3) & 3
	dataBytes := (uint(buf[1]) & 7)

	hlen := 2 + msgTypeBytes + msgIdBytes
	hlen += fwdBytes + bodyBytes + dataBytes
	//if len(buf) < hlen: raise IncompleteIpcMsgError()

	offset := uint(2)

	*msgType = DecodeUInt(buf, offset, msgTypeBytes)
	offset += msgTypeBytes

	*msgId = DecodeUInt64(buf, offset, msgIdBytes)
	offset += msgIdBytes

	*fwdLen = DecodeUInt(buf, offset, fwdBytes)
	offset += fwdBytes

	*bodyLen = DecodeUInt(buf, offset, bodyBytes)
	offset += bodyBytes

	*dataLen = DecodeUInt(buf, offset, dataBytes)
	offset += dataBytes

	return offset
}

func main() {
	var pkgType, msgType, fwdLen, bodyLen, dataLen uint
	var msgId uint64

	conn, _ := net.Dial("tcp", "localhost:11125")

	pkg := BuildMsgHead(1, 20, 50, 3, 4, 5)
	println("pkg len", len(pkg))
	_, _ = conn.Write([]byte(pkg))
	conn.Write([]byte("fwd"))
	conn.Write([]byte("body"))
	conn.Write([]byte("data1"))

	reply := make([]byte, 1024)
	_, _ = conn.Read(reply)
	off := ParseMsgHead(pkg, &pkgType, &msgType, &msgId, &fwdLen, &bodyLen, &dataLen)
	fmt.Printf("off=%v pkgType=%v msgType=%v msgId=%v fwdLen=%v bodyLen=%v dataLen=%v\n",
		off, pkgType, msgType, msgId, fwdLen, bodyLen, dataLen)
	println(string(reply[off:]))
}
