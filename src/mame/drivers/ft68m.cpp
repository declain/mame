// license:BSD-3-Clause
// copyright-holders:Robbbert
/***************************************************************************

Forward Technology FT-68M Multibus card.

2013-09-26 Skeleton driver

Chips: HD68000-10, uPD7201C, AM9513APC. Crystal: 19.6608 MHz

Interrupts: INT6 is output of Timer 2, INT7 is output of Timer 3 (refresh),
            INT5 comes from SIO.

****************************************************************************/

#include "emu.h"
#include "cpu/m68000/m68000.h"
#include "bus/rs232/rs232.h"
#include "machine/am9513.h"
#include "machine/z80sio.h"

class ft68m_state : public driver_device
{
public:
	ft68m_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
		m_p_base(*this, "rambase"),
		m_maincpu(*this, "maincpu")
	{
	}

	DECLARE_READ16_MEMBER(switches_r);

private:
	virtual void machine_start() override;
	virtual void machine_reset() override;

	required_shared_ptr<uint16_t> m_p_base;

	required_device<cpu_device> m_maincpu;
};

READ16_MEMBER( ft68m_state::switches_r )
{
	return 0x7c00; // bypass self test
}


static ADDRESS_MAP_START(ft68m_mem, AS_PROGRAM, 16, ft68m_state)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xffffff)
	AM_RANGE(0x000000, 0x1fffff) AM_RAM AM_SHARE("rambase")
	AM_RANGE(0x200000, 0x201fff) AM_ROM AM_REGION("roms", 0x0000)
	AM_RANGE(0x400000, 0x401fff) AM_ROM AM_REGION("roms", 0x2000)
	AM_RANGE(0x600000, 0x600007) AM_MIRROR(0x1ffff8) AM_DEVREADWRITE8("mpsc", upd7201_new_device, ba_cd_r, ba_cd_w, 0xff00)
	AM_RANGE(0x800000, 0x800003) AM_MIRROR(0x1ffffc) AM_DEVREADWRITE("stc", am9513_device, read16, write16)
	AM_RANGE(0xa00000, 0xbfffff) AM_RAM //Page Map
	AM_RANGE(0xc00000, 0xdfffff) AM_RAM //Segment Map
	AM_RANGE(0xe00000, 0xffffff) AM_READ(switches_r) //Context Register
ADDRESS_MAP_END


/* Input ports */
static INPUT_PORTS_START( ft68m )
INPUT_PORTS_END


void ft68m_state::machine_start()
{
	// GATE 1 is tied to Vcc; other GATE and SRC pins are all grounded
	subdevice<am9513_device>("stc")->gate1_w(1);
}

void ft68m_state::machine_reset()
{
	uint8_t* ROM = memregion("roms")->base();
	memcpy(m_p_base, ROM, 8);
	m_maincpu->reset();
}

static MACHINE_CONFIG_START( ft68m )

	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", M68000, XTAL_19_6608MHz / 2)
	MCFG_CPU_PROGRAM_MAP(ft68m_mem)

	MCFG_DEVICE_ADD("mpsc", UPD7201_NEW, 0)
	MCFG_Z80SIO_OUT_TXDA_CB(DEVWRITELINE("rs232a", rs232_port_device, write_txd))
	MCFG_Z80SIO_OUT_DTRA_CB(DEVWRITELINE("rs232a", rs232_port_device, write_dtr))
	MCFG_Z80SIO_OUT_RTSA_CB(DEVWRITELINE("rs232a", rs232_port_device, write_rts))
	MCFG_Z80SIO_OUT_TXDB_CB(DEVWRITELINE("rs232b", rs232_port_device, write_txd))
	MCFG_Z80SIO_OUT_INT_CB(INPUTLINE("maincpu", M68K_IRQ_5))

	MCFG_DEVICE_ADD("stc", AM9513A, XTAL_19_6608MHz / 8)
	MCFG_AM9513_OUT2_CALLBACK(INPUTLINE("maincpu", M68K_IRQ_6))
	MCFG_AM9513_OUT3_CALLBACK(INPUTLINE("maincpu", M68K_IRQ_7))
	MCFG_AM9513_OUT4_CALLBACK(DEVWRITELINE("mpsc", upd7201_new_device, rxca_w))
	MCFG_DEVCB_CHAIN_OUTPUT(DEVWRITELINE("mpsc", upd7201_new_device, txca_w))
	MCFG_AM9513_OUT5_CALLBACK(DEVWRITELINE("mpsc", upd7201_new_device, rxcb_w))
	MCFG_DEVCB_CHAIN_OUTPUT(DEVWRITELINE("mpsc", upd7201_new_device, txcb_w))

	MCFG_RS232_PORT_ADD("rs232a", default_rs232_devices, "terminal")
	MCFG_RS232_RXD_HANDLER(DEVWRITELINE("mpsc", upd7201_new_device, rxa_w))
	MCFG_RS232_DSR_HANDLER(DEVWRITELINE("mpsc", upd7201_new_device, dcda_w))
	MCFG_RS232_CTS_HANDLER(DEVWRITELINE("mpsc", upd7201_new_device, ctsa_w))

	MCFG_RS232_PORT_ADD("rs232b", default_rs232_devices, nullptr)
	MCFG_RS232_RXD_HANDLER(DEVWRITELINE("mpsc", upd7201_new_device, rxb_w))
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( ft68m )
	ROM_REGION16_BE(0x4000, "roms", 0)
	ROM_LOAD16_BYTE("23-0009-01c.a4", 0x0000, 0x1000, CRC(0d45fc8d) SHA1(59587cb1c151bfd0d69e708716ed3b0a78aa85ea) )
	ROM_LOAD16_BYTE("23-0008-01c.a1", 0x0001, 0x1000, CRC(d1aa1164) SHA1(05e10f1c594e2acd369949b873a524a9cc37829f) )
	ROM_LOAD16_BYTE( "33-01.a6", 0x2000, 0x1000, CRC(53fe3c73) SHA1(ad15c74cd8edef9d9716ad0d16f7a95ff2af901f) )
	ROM_LOAD16_BYTE( "33-00.a3", 0x2001, 0x1000, CRC(06b1cc77) SHA1(12e3314e92f800b3c4ebdf55dcd5351230224788) )

	ROM_REGION(0x700, "proms", 0)
	ROM_LOAD("23-0010-00.a15", 0x000, 0x020, CRC(20eb1183) SHA1(9b268792b28d858d6b6a1b6c4148af88a8d6b735) )
	ROM_LOAD("23-0011-00.a14", 0x100, 0x200, CRC(12d9a6be) SHA1(fca99f9c5afc630ac67cbd4e5ba4e5242b826848) )
	ROM_LOAD("23-0012-00.a16", 0x300, 0x020, CRC(ee1e5a14) SHA1(0d3346cb3b647fa2475bd7b4fa36ea6ecfdaf805) )
	ROM_LOAD("23-0034-00.e4",  0x400, 0x100, CRC(1a573887) SHA1(459bd2d8dc8c4b1c0a529984ae8e38d0c81a084c) )
	ROM_LOAD("23-0037-00.e7",  0x500, 0x100, CRC(9ed4b7f6) SHA1(136a74567094d8462c3a4de1b7e6eb8f30fe71ca) )
	ROM_LOAD("23-0038-00.f1",  0x600, 0x100, CRC(3e56cce5) SHA1(f30a8d5d744bfc25493cd1e92961bbb75f9e0d05) )
ROM_END


/* Driver */

//    YEAR  NAME   PARENT  COMPAT  MACHINE INPUT   CLASS        INIT  COMPANY               FULLNAME  FLAGS
COMP( 198?, ft68m, 0,      0,      ft68m,  ft68m,  ft68m_state, 0,    "Forward Technology", "FT-68M", MACHINE_NOT_WORKING | MACHINE_NO_SOUND_HW )
