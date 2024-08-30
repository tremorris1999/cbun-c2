const uuidRegex = /^[A-F\d]{8}\-(?:[A-F\d]{4}\-){3}[A-F\d]{12}$/i
export const UUID = {
  test: (value?: string) => uuidRegex.test(value || '')
}

export type UUID = ReturnType<typeof crypto.randomUUID> | typeof UUID
